/***********************************************
 This file is part of the NRTB project (https://launchpad.net/nrtb).
 
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
 
#ifndef sim_db_channel_header
#define sim_db_channel_header

#include <common.h>
#include <base_thread.h>
#include <circular_queue.h>
#include <transceiver.h>
#include <triad.h>
#include <sim_to_db_wrapper.pb.h>
#include <boost/shared_ptr.hpp>

namespace nrtb
{

class sim_db_channel
{
public:
  
  // -- typedefs to be used by callers
  typedef triad<long double> triplet;
  //  individual object setup info
  struct obj_setup_info
  {
	uint32_t uid;
	std::string name;
	uint32_t obj_type;
	triplet location;
	triplet attitude;
	triplet velocity;
	triplet rotation;    
  };
  // typedef a list of obj_setup_infos
  typedef std::list<obj_setup_info> obj_setup_list;
  // overall simulation info
  struct sim_info
  {
	uint32_t uid;
	std::string name;
	uint32_t quanta_ms:
	uint32_t max_quantas;
	std::string started_by;
	obj_setup_list items;
  };
  // obj per instant status info
  struct tq_obj_data
  {
	uint32 uid;
	triplet location;
	triplet attitude;
	triplet velocity;
	triplet rotation;  
	strlist in_msgs;
	strlist out_msgs;
	strlist notes;
  };
  typedef std::list<tq_obj_data> tq_obj_updates;
  // time quanta update record
  struct tq_update
  {
	uint32 uid;
	uint32 quanta_index;
	uint32 actual_ms;
	uint32 cook_ms;
	tq_obj_updates updates;
	obj_setup_list new_items;
  };
  
  /***************************************************
	in alpha phase, all exceptions will force the channel 
	back to it's initial state, closing any connection
	which may be active at the time.
  ***************************************************/
  
  // general parent for all class exceptions
  class general_exception: public base_exception {};
  class cant_connect_exception: public general_exception {};
  class unrecoverable_com_error: public general_exception {};
  class local_invalid_context: public general_exception {};
  class remote_invalid_context: public general_exception {};
  class in_buffer_overflow: public general_exception {};
  class out_buffer_overflow: public general_exception {};
  
  // Note: the constructor expects to find all needed
  // parameters in a nrtb::global_conf_reader object.
  sim_db_channel();
  // this will attempt to gracefuly close the connection
  virtual ~sim_db_channel();
  
  /*************************************************
	Called to open the connection and do any 
	link negociation which may be required.
	If address is provided it will be used, otherwise
	the global_conf_reader will be queried for one.
	if none is available, this class will throw. 
  **************************************************/
  void establish_link(std::string address = "na");
  
  /*****************************************************
	Called to notify the databroker that a new simulation
	is starting.
	Must be called after establish_link() and only once
	at the beginning of each simulation or an
	local_invalid_context exception will be thrown.
	Any throws will cause the connection to be closed 
	and the channel returned to its initial state.
  *****************************************************/
  void sim_start(sim_info data);
  
  /************************************************
	* this is called to send the results of a 
	* single time quanta's updates the the databroker.
	* 
	* This will throw if not called after establish_link
	* and sim_start and before sim_end. In that case
	* the channel will be returne to it's initial
	* state
  ************************************************/
  void sim_update(tq_update data); 
  
  /************************************************
	* Called ot notify the databroker of the end of
	* a simulation run. 
	* 	failed: true if the sim errored out.
	* 	reason: a human readable cause
	* 
	* This will throw if not called after establish_link
	* and sim_start and before sim_end. In that case
	* the channel will be returne to it's initial
	* state
  ************************************************/
  void sim_end(bool failed, std::string reason);
  
  /*************************************************
	* Call this to notify the databroker that we are 
	* closing the connection and then actually close 
	* it.
	* 
	* This will throw if a simulation is in progress.
	* Either way, the channel will be returned to 
	* its initial state.
  *************************************************/
  void end_link();
	
	
protected:
  typedef boost::shared_ptr<thread> thread_p;
  typedef nrtb_msg::sim_to_db_wrapper msg_t;
  typedef transceiver<msg_t, msg_t> io_t;
  typedef boost::shared_ptr<io_t> io_p;
  typedef boost::shared_ptr<msg_t> msg_p;
  typedef circular_queue<msg_p> msg_buff;
  typedef boost::shared_ptr<msg_buff> msg_buff_p;
  thread_p input_manager;
  thread_p output_manager;
  io_p link;
  msg_buff_p out_queue;
  msg_buff_p in_queue;
  unsigned int in_queue_limit, out_queue_limit;
  // the other methods call this in case of error.
  void return_to_base_state();
  class in_processor: thread
  {
	in_processor(msg_buff_p buf, io_p io);
	virtual ~in_processor() {};
	run();
  };

  class out_processor: thread
  {
	out_processor(msg_buff_p buf, io_p io);
	virtual ~out_processor() {};
	run();
  };
  
};

} // namepace nrtb

#endif // sim_db_channel_header