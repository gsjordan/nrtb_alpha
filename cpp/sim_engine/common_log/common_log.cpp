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

// see base_socket.h for documentation

#include "common_log.h"
#include <memory>
#include <sstream>

using namespace nrtb;

log_queue log_factory::clq;

log_file_writer log_factory::writer(clq, "sim_engine.log");

log_recorder log_factory::operator () (std::string s)
{
  log_recorder returnme(s,clq);
  return returnme;
};
