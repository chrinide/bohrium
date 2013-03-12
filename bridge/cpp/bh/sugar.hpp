/*
This file is part of Bohrium and copyright (c) 2012 the Bohrium team:
http://bohrium.bitbucket.org

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
#ifndef __BOHRIUM_BRIDGE_CPP_SUGAR
#define __BOHRIUM_BRIDGE_CPP_SUGAR

namespace bh {

template <typename T>
void pprint(multi_array<T> & operand)
{
    bool first = true;

    multi_array<double>::iterator it  = operand.begin();
    multi_array<double>::iterator end = operand.end();

    std::cout << "[ ";
    for(; it != end; it++) {

        if (!first) {
            std::cout  << ", ";
        } else {
            first = false;
        }
        std::cout << *it;
    }
    std::cout << " ]" << std::endl;
}

template <typename T>
bool equal(multi_array<T>& i, multi_array<T> j)
{
    return false;
}

template <typename T>
bool equivalent(multi_array<T>& i, multi_array<T> j)
{
    return false;
}

}
#endif
