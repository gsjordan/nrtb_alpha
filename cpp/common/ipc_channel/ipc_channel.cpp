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

#include "ipc_channel.h"

namespace nrtb 
{
abs_ipc_record::abs_ipc_record(ipc_queue& q): 
  return_to(q) {};

ipc_queue& ipc_channel_manager::get(std::string name)
{
  return channels[name];
};
  
ipc_channel_manager::iterator ipc_channel_manager::begin()
{
  return channels.begin();
};

ipc_channel_manager::iterator ipc_channel_manager::end()
{
  return channels.end();
}

} // namespace nrtb

