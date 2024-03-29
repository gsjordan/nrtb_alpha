/***********************************************
 This file is part of the NRTB project (https://github.com/fpstovall/nrtb_alpha).
 
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

#include "common_log.h"
#include <iostream>
#include <string>

using namespace nrtb;
using namespace std;

int main()
{
  bool failed = true;
  cout << "================ common log test ================="
    << endl;

  try 
  {
    log_recorder mylog(common_log::get_reference()("Unit_test"));
    mylog.trace("Unit test start");
    mylog.trace("Unit test complete");
    failed = false;
  }
  catch (...) 
  {
    cerr << " ** An exception was caught **" << endl;
  };    

  cout << "=========== common log test complete ============="
    << endl;
  
  return failed;
};



























