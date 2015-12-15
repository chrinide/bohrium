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
#include <bh.h>
#include <bh_fuse_cache.h>
#include <iostream>
#include <fstream>
#include <boost/foreach.hpp>
#include <vector>
#include <map>
#include <stdexcept>

using namespace std;
using namespace boost;
using namespace bohrium;

static void do_fusion(bh_ir &bhir)
{
    for(uint64_t idx=0; idx < bhir.instr_list.size(); idx++)
    {
        bh_ir_kernel kernel(bhir);
        kernel.add_instr(idx);
        bhir.kernel_list.push_back(kernel);
    }
}

void fuser(bh_ir &bhir, FuseCache &cache)
{
    if(bhir.kernel_list.size() != 0)
        throw logic_error("The kernel_list is not empty!");

    if(cache.enabled)
    {
        BatchHash hash(bhir.instr_list);
        if(cache.lookup(hash, bhir, bhir.kernel_list))
            return;//Fuse cache hit!
        do_fusion(bhir);
        cache.insert(hash, bhir.kernel_list);
    }
    else
    {
        do_fusion(bhir);
    }
}

