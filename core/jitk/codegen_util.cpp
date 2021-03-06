/*
This file is part of Bohrium and copyright (c) 2012 the Bohrium
team <http://www.bh107.org>.

Bohrium is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation, either version 3
of the License, or (at your option) any later version.

Bohrium is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the
GNU Lesser General Public License along with Bohrium.

If not, see <http://www.gnu.org/licenses/>.
*/

#include <jitk/codegen_util.hpp>
#include <jitk/view.hpp>
#include <jitk/instruction.hpp>

using namespace std;

namespace bohrium {
namespace jitk {


// Does 'instr' reduce over the innermost axis?
// Notice, that such a reduction computes each output element completely before moving
// to the next element.
bool sweeping_innermost_axis(InstrPtr instr) {
    if (not bh_opcode_is_sweep(instr->opcode))
        return false;
    assert(instr->operand.size() == 3);
    return instr->sweep_axis() == instr->operand[1].ndim-1;
}

void write_loop_block(const SymbolTable &symbols,
                      const Scope *parent_scope,
                      const LoopB &block,
                      const ConfigParser &config,
                      const vector<const LoopB *> &threaded_blocks,
                      bool opencl,
                      std::function<const char *(bh_type type)> type_writer,
                      std::function<void (const SymbolTable &symbols,
                                          Scope &scope,
                                          const LoopB &block,
                                          const ConfigParser &config,
                                          bool loop_is_peeled,
                                          const std::vector<const LoopB *> &threaded_blocks,
                                          std::stringstream &out)> head_writer,
                      std::stringstream &out) {

    if (block.isSystemOnly()) {
        out << "// Removed loop with only system instructions\n";
        return;
    }
    spaces(out, 4 + block.rank*4);

    // Let's find the local temporary arrays and the arrays to scalar replace
    const set<bh_base *> &local_tmps = block.getLocalTemps();

    // Let's scalar replace reduction outputs that reduces over the innermost axis
    vector<const bh_view*> scalar_replaced_reduction_outputs;
    for (const InstrPtr instr: block._sweeps) {
        if (bh_opcode_is_reduction(instr->opcode) and sweeping_innermost_axis(instr)) {
            if (local_tmps.find(instr->operand[0].base) == local_tmps.end() and
                    (parent_scope == NULL or parent_scope->isArray(instr->operand[0]))) {
                scalar_replaced_reduction_outputs.push_back(&instr->operand[0]);
            }
        }
    }

    // Let's scalar replace input-only arrays that are used multiple times
    vector<const bh_view*> scalar_replaced_input_only;
    {
        const vector<InstrPtr> block_instr_list = block.getAllInstr();
        // We have to ignore output arrays and arrays that are accumulated
        set<bh_base *> ignore_bases;
        for (const InstrPtr &instr: block_instr_list) {
            if (instr->operand.size() > 0) {
                ignore_bases.insert(instr->operand[0].base);
            }
            if (bh_opcode_is_accumulate(instr->opcode)) {
                ignore_bases.insert(instr->operand[1].base);
            }
        }
        // First we add a valid view to the set of 'candidates' and if we encounter the view again
        // we add it to the 'scalar_replaced_input_only'
        set<bh_view> candidates;
        for (const InstrPtr &instr: block_instr_list) {
            for(size_t i=1; i < instr->operand.size(); ++i) {
                const bh_view &input = instr->operand[i];
                if ((not bh_is_constant(&input)) and ignore_bases.find(input.base) == ignore_bases.end()) {
                    if (local_tmps.find(input.base) == local_tmps.end() and
                        (parent_scope == NULL or parent_scope->isArray(input))) {
                        if (util::exist(candidates, input)) { // 'input' is used multiple times
                            scalar_replaced_input_only.push_back(&input);
                        } else {
                            candidates.insert(input);
                        }
                    }
                }
            }
        }
    }

    // And then create the scope
    Scope scope(symbols, parent_scope, local_tmps, scalar_replaced_reduction_outputs,
                scalar_replaced_input_only, config);

    // When a reduction output is a scalar (e.g. because of array contraction or scalar replacement),
    // it should be declared before the for-loop
    for (const InstrPtr instr: block._sweeps) {
        if (bh_opcode_is_reduction(instr->opcode)) {
            const bh_view &output = instr->operand[0];
            if (not scope.isDeclared(output) and not scope.isArray(output)) {
                // Let's write the declaration of the scalar variable
                scope.writeDeclaration(output, type_writer(output.base->type), out);
                out << "\n";
                spaces(out, 4 + block.rank * 4);
            }
        }
    }

    // Find indexes we will declare later
    vector<const bh_view*> indexes;
    {
        set<bh_view, idx_less> candidates;
        for (const InstrPtr &instr: block.getLocalInstr()) {
            for (const bh_view* view: instr->get_views()) {
                if (symbols.existIdxID(*view) and scope.isArray(*view)) {
                    if (util::exist(candidates, *view)) { // 'view' is used multiple times
                        indexes.push_back(view);
                    } else {
                        candidates.insert(*view);
                    }
                }
            }
        }
    }

    // We might not have to loop "peel" if all reduction have an identity value and writes to a scalar
    bool need_to_peel = false;
    {
        for (const InstrPtr instr: block._sweeps) {
            const bh_view &v = instr->operand[0];
            if (not (has_reduce_identity(instr->opcode) and (scope.isScalarReplaced(v) or scope.isTmp(v.base)))) {
                need_to_peel = true;
                break;
            }
        }
    }

    // When not peeling, we need a neutral initial reduction value
    if (not need_to_peel) {
        for (const InstrPtr instr: block._sweeps) {
            const bh_view &view = instr->operand[0];
            if (not scope.isArray(view) and not scope.isDeclared(view)) {
                scope.writeDeclaration(view, type_writer(view.base->type), out);
                out << "\n";
                spaces(out, 4 + block.rank * 4);
            }
            scope.getName(view, out);
            out << " = ";
            write_reduce_identity(instr->opcode, view.base->type, out);
            out << ";\n";
            spaces(out, 4 + block.rank * 4);
        }
    }

    // If this block is sweeped, we will "peel" the for-loop such that the
    // sweep instruction is replaced with BH_IDENTITY in the first iteration
    if (block._sweeps.size() > 0 and need_to_peel) {
        Scope peeled_scope(scope);
        LoopB peeled_block(block);
        for (const InstrPtr instr: block._sweeps) {
            // The input is the same as in the sweep
            bh_instruction sweep_instr(BH_IDENTITY, {instr->operand[0], instr->operand[1]});

            // But the output needs an extra dimension when we are reducing to a non-scalar
            if (bh_opcode_is_reduction(instr->opcode) and instr->operand[1].ndim > 1) {
                sweep_instr.operand[0].insert_axis(instr->constant.get_int64(), 1, 0);
            }
            peeled_block.replaceInstr(instr, sweep_instr);
        }
        string itername;
        {stringstream t; t << "i" << block.rank; itername = t.str();}
        out << "{ // Peeled loop, 1. sweep iteration\n";
        spaces(out, 8 + block.rank*4);
        out << type_writer(BH_UINT64) << " " << itername << " = 0;\n";

        // Write temporary and scalar replaced array declarations
        for (const InstrPtr instr: block.getLocalInstr()) {
            for (const bh_view *view: instr->get_views()) {
                if (not peeled_scope.isDeclared(*view)) {
                    if (peeled_scope.isTmp(view->base)) {
                        spaces(out, 8 + block.rank * 4);
                        peeled_scope.writeDeclaration(*view, type_writer(view->base->type), out);
                        out << "\n";
                    } else if (peeled_scope.isScalarReplaced_R(*view)) {
                        spaces(out, 8 + block.rank * 4);
                        peeled_scope.writeDeclaration(*view, type_writer(view->base->type), out);
                        out << " " << peeled_scope.getName(*view) << " = a" << symbols.baseID(view->base);
                        write_array_subscription(peeled_scope, *view, out);
                        out << ";";
                        out << "\n";
                    }
                }
            }
        }
        // Write the indexes declarations
        for (const bh_view *view: indexes) {
            if (not peeled_scope.isIdxDeclared(*view)) {
                spaces(out, 8 + block.rank * 4);
                peeled_scope.writeIdxDeclaration(*view, type_writer(BH_UINT64), out);
                out << "\n";
            }
        }
        out << "\n";
        for (const Block &b: peeled_block._block_list) {
            if (b.isInstr()) {
                if (b.getInstr() != NULL) {
                    spaces(out, 4 + b.rank()*4);
                    write_instr(peeled_scope, *b.getInstr(), out, opencl);
                }
            } else {
                write_loop_block(symbols, &peeled_scope, b.getLoop(), config, threaded_blocks, opencl, type_writer, head_writer, out);
            }
        }
        spaces(out, 4 + block.rank*4);
        out << "}\n";
        spaces(out, 4 + block.rank*4);
    }

    // Write the for-loop header
    head_writer(symbols, scope, block, config, need_to_peel, threaded_blocks, out);

    // Write temporary and scalar replaced array declarations
    for (const InstrPtr instr: block.getLocalInstr()) {
        for (const bh_view *view: instr->get_views()) {
            if (not scope.isDeclared(*view)) {
                if (scope.isTmp(view->base)) {
                    spaces(out, 8 + block.rank * 4);
                    scope.writeDeclaration(*view, type_writer(view->base->type), out);
                    out << "\n";
                } else if (scope.isScalarReplaced_R(*view)) {
                    spaces(out, 8 + block.rank * 4);
                    scope.writeDeclaration(*view, type_writer(view->base->type), out);
                    out << " " << scope.getName(*view) << " = a" << symbols.baseID(view->base);
                    write_array_subscription(scope, *view, out);
                    out << ";";
                    out << "\n";
                }
            }
        }
    }
    // Write the indexes declarations
    for (const bh_view *view: indexes) {
        if (not scope.isIdxDeclared(*view)) {
            spaces(out, 8 + block.rank * 4);
            scope.writeIdxDeclaration(*view, type_writer(BH_UINT64), out);
            out << "\n";
        }
    }

    // Write the for-loop body
    // The body in OpenCL and OpenMP are very similar but OpenMP might need to insert "#pragma omp atomic/critical"
    if (opencl) {
        for (const Block &b: block._block_list) {
            if (b.isInstr()) { // Finally, let's write the instruction
                if (b.getInstr() != NULL) {
                    spaces(out, 4 + b.rank()*4);
                    write_instr(scope, *b.getInstr(), out, true);
                }
            } else {
                write_loop_block(symbols, &scope, b.getLoop(), config, threaded_blocks, opencl, type_writer, head_writer, out);
            }
        }
    } else {
        for (const Block &b: block._block_list) {
            if (b.isInstr()) { // Finally, let's write the instruction
                const InstrPtr instr = b.getInstr();
                if (instr->operand.size() > 0 and not bh_opcode_is_system(instr->opcode)) {
                    if (scope.isOpenmpAtomic(instr->operand[0])) {
                        spaces(out, 4 + b.rank()*4);
                        out << "#pragma omp atomic\n";
                    } else if (scope.isOpenmpCritical(instr->operand[0])) {
                        spaces(out, 4 + b.rank()*4);
                        out << "#pragma omp critical\n";
                    }
                }
                spaces(out, 4 + b.rank()*4);
                write_instr(scope, *instr, out);
            } else {
                write_loop_block(symbols, &scope, b.getLoop(), config, threaded_blocks, opencl, type_writer, head_writer, out);
            }
        }
    }
    spaces(out, 4 + block.rank*4);
    out << "}\n";

    // Let's copy the scalar replaced reduction outputs back to the original array
    for (const bh_view *view: scalar_replaced_reduction_outputs) {
        spaces(out, 4 + block.rank*4);
        out << "a" << symbols.baseID(view->base);
        write_array_subscription(scope, *view, out, true);
        out << " = ";
        scope.getName(*view, out);
        out << ";\n";
    }
}


} // jitk
} // bohrium
