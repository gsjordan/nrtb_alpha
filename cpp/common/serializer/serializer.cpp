/***********************************************
 T his file is part of the NRTB project (https://github.com/fpstovall/nrtb_alpha).
 
 NRTB is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 NRTB is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with NRTB.  If not, see <http://www.gnu.org/licenses/>.
 
 **********************************************/
 
#include "serializer.h"

using namespace nrtb;

nrtb::serializer::serializer()
{
  counter = 0;
};

nrtb::serializer::serializer(unsigned long long start)
{
  counter = start;
};

serializer::~serializer()
{
  // nop destructor 
};

unsigned long long serializer::operator()()
{
  return counter++;
}
