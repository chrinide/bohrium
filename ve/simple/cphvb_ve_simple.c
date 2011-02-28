/*
 * Copyright 2011 Mads R. B. Kristensen <madsbk@gmail.com>
 *
 * This file is part of cphVB.
 *
 * cphVB is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cphVB is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cphVB.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <cphvb_ve_simple.h>


cphvb_error cphvb_ve_simple_init(cphvb_intp *opcode_count,
                                 cphvb_opcode opcode_list[CPHVB_MAX_NO_OPERANDS],
                                 cphvb_intp *datatype_count,
                                 cphvb_type datatype_list[CPHVB_NO_TYPES])
{
    opcode_list[0] = CPHVB_ADD;
    *opcode_count = 1;

    datatype_list[0] = CPHVB_FLOAT64;
    *datatype_count = 1;
    return CPHVB_SUCCESS;
}


cphvb_error cphvb_ve_simple_shutdown(void)
{
    return CPHVB_SUCCESS;
}


cphvb_error cphvb_ve_simple_execute(cphvb_intp instruction_count,
                                    cphvb_instruction instruction_list[CPHVB_MAX_NO_OPERANDS])
{
    cphvb_intp i;
    for(i=0; i<instruction_count; ++i)
    {
        cphvb_instruction *inst = &instruction_list[i];
        switch(inst->opcode)
        {
        case CPHVB_ADD:
        {
            cphvb_intp j;
            cphvb_float64 *d0, *d1, *d2;
            cphvb_array *a0 = inst->operand[0];
            cphvb_array *a1 = inst->operand[1];
            cphvb_array *a2 = inst->operand[2];

            d0 = cphvb_base_array(inst->operand[0])->data;
            if(a1 == CPHVB_CONSTANT)
                d1 = (cphvb_float64*) &inst->constant[1];
            else
                d1 = cphvb_base_array(inst->operand[1])->data;

            if(a2 == CPHVB_CONSTANT)
                d2 = (cphvb_float64*) &inst->constant[2];
            else
                d2 = cphvb_base_array(inst->operand[2])->data;

            if(!(cphvb_malloc_array_data(a0) == CPHVB_SUCCESS &&
                 cphvb_malloc_array_data(a1) == CPHVB_SUCCESS &&
                 cphvb_malloc_array_data(a2) == CPHVB_SUCCESS))
            {
                fprintf(stderr,"Out of memory applying CPHVB_ADD\n");
                exit(CPHVB_OUT_OF_MEMORY);
            }

            for (j=0; j<cphvb_nelements(a0->ndim, a0->shape); ++j)
            {
                cphvb_intp off0, off1=0, off2=0;
                off0 = cphvb_calc_offset(a0->ndim, a0->shape, a0->stride, j);
                if(a1 != CPHVB_CONSTANT)
                    off1 = cphvb_calc_offset(a1->ndim, a1->shape, a1->stride, j);
                if(a2 != CPHVB_CONSTANT)
                    off2 = cphvb_calc_offset(a2->ndim, a2->shape, a2->stride, j);
                *(d0 + off0) = *(d1 + off1) + *(d2 + off2);
            }

            break;
        }
        default:
            fprintf(stderr, "cphvb_ve_simple_execute() encountered an "
                            "unknown opcode: %s.",
                            cphvb_opcode_text(inst->opcode));
            exit(CPHVB_INST_NOT_SUPPORTED);
        }
    }

    return CPHVB_SUCCESS;
}
