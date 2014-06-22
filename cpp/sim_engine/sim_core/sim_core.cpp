/***********************************************
 This file is part of the NRTB project (https://*launchpad.net/nrtb).
 
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

#include "sim_core.h"
#include <unistd.h>
#include <hires_timer.h>
#include <ipc_channel.h>
#include <common_log.h>
#include <sstream>

using namespace std;
using namespace nrtb;

sim_core::sim_core(float time_slice)
  : quanta(0), 
    quanta_duration(time_slice),
    end_run(false),
    is_running(false)
{};

strlist sim_core::obj_status()
{
  strlist returnme;
  for(auto o: all_objects)
  {
    returnme.push_back(o.second->as_str());
  };
  return returnme;
};

const object_list sim_core::get_obj_copies()
{
  object_list returnme;
  for(auto o: all_objects)
  {
     const object_p t = o.second;
     returnme[o.first] = t;
  };
  return returnme;
};

void sim_core::tick(unsigned long long quanta)
{
  // call the local tick and apply for each object in the simulation.
  for(auto & a: all_objects)
  {
    // have to use this method to make sure the apply step is not
    // optimized out by the compiler in some conditions.
    bool killme = a.second->tick(quanta);
    bool killme2 = a.second->apply(quanta, quanta_duration);
    // mark the object for deletion if appropriate.
    if (killme or killme2)
      deletions.push_back(a.first);
  };
  // at the end of this method, all objects are either
  // at their final state ignorning collisions or deleted.
};

void sim_core::collision_check()
{
  // this version is using a very naive algorithm.
  auto b = all_objects.begin();
  auto c = b;
  auto e = all_objects.end();
  while (c != e)
  {
    b = c;
    b++;
    while (b != e)
    {
      if (c->second->check_collision(b->second))
      {
        // a collision has been found.
        clsn_rec crec;
        crec.a = c->first;
        crec.b = b->first;
        collisions.push_back(crec);
      };
      b++;
    };
    c++;
  };
  // at exit, all colliions have been recorded.
};


void sim_core::turn_init(unsigned long long quanta)
{
  // Process all the messages in queue at this point.
  for(int i=messages.size();i>0;i--)
  {
    ipc_record_p raw(messages.pop());
    gp_sim_message_p msg(static_cast<gp_sim_message *>(raw.release()));
    // check for proper message type
    if (msg->msg_type() != 0)
    {
      // We'll treat this as a hard error.
      stringstream s;
      s << "bad msg in sim_core::turn_init(): "
	      << msg->as_str();
      base_exception e;
      e.store(s.str());
      throw e;
    };
    const int noun_obj(1);
    const int noun_ctl(2);
    switch (msg->noun())
    {
      case noun_obj:
      {
      	const int verb_add(1);
      	const int verb_rm(2);
      	switch (msg->verb())
      	{
      	  case verb_add:
      	  {
      	    // Add new object to list.
      	    // TODO: We may want to check for unique id.
      	    auto new_obj= msg->data<object_p>();
      	    all_objects[new_obj->id] = new_obj;
      	    break;
      	  }
      	  case verb_rm:
      	  {
      	    // maark object for deletion.
      	    auto did=msg->data<unsigned long long>();
      	    deletions.push_back(did);
      	    break;
      	  }
      	  default:
      	  {
      	    // unhandled message
      	    stringstream s;
      	    s << "Unhanded object verb in sim_core::turn_init: "
      	      << msg->as_str();
      	    base_exception e;
      	    e.store(s.str());
      	    throw e;
      	    break;
      	  };
      	};
      	break;
      }
      case noun_ctl:
      {
      	const int verb_stop(1);
      	switch (msg->verb())
      	{
      	  case verb_stop:
      	  {
            // TODO: make sure this flag is checked in caller.
            // TODO: (it's not written yet.)
            end_run = true;
      	    break;
      	  }
      	  default:
      	  {
      	    // unhandled message
      	    stringstream s;
      	    s << "Unhanded control verb in sim_core::turn_init: "
      	      << msg->as_str();
      	    base_exception e;
      	    e.store(s.str());
      	    throw e;	
      	    break;
      	  }
      	}
      }
      default:
      {
      	// unhandled noun; treat as a hard error.
      	stringstream s;
      	s << "bad noun in sim_core::turn_init(): "
      	  << msg->as_str();
      	base_exception e;
      	e.store(s.str());
      	throw e;
      	break;
      }
    };
  };
  /* conduct all cleanup needed to ensure the sim
     is a proper state for the next turn. */
  // Clear out the collision list
  collisions.clear();
  // delete any objects marked in the last turn.
  for (auto a : deletions)
  {
    // ignore errors here.
    try { all_objects.erase(a); } catch (...) {};
  };
  // clear the deletions list
  deletions.clear();
};

void sim_core::put_message(gp_sim_message_p m)
{
  ipc_record_p t(static_cast<abs_ipc_record *>(m.release()));
  messages.push(std::move(t));
};

gp_sim_message_p sim_core::next_out_message()
{
  ipc_record_p t(messages.pop());
  gp_sim_message_p g(static_cast<gp_sim_message *>(t.release()));
  return std::move(g);
};

void sim_core::add_object(object_p obj)
{
  // assemble the message.
  void_p d(new object_p(obj));
  gp_sim_message_p g(new gp_sim_message(messages,0,1,1,d));
  // queue the message.
  put_message(std::move(g));
};

void sim_core::remove_obj(long long unsigned int oid)
{
  // assemble the message
  void_p d(new unsigned long long(oid));
  gp_sim_message_p g(new gp_sim_message(messages,0,1,2,d));
  // queue the message.
  put_message(std::move(g));
};

void sim_core::stop_sim()
{
  // assemble the message
  gp_sim_message_p g(new gp_sim_message(messages,0,2,1));
  // queue the message.
  put_message(std::move(g));
};

void sim_core::run_sim(sim_core& world)
{
  world.is_running = true;
  // link to sim engine general log
  log_recorder glog(common_log::get_reference()("sim_core::run"));
  glog.trace("Starting");
  try
  {
    // establish output links.
    // -- sim_core output channel
    ipc_channel_manager& ipc
      = global_ipc_channel_manager::get_reference();
    ipc_queue & output = ipc.get("sim_output");
    // output initial state
    glog.trace("Storing inital model state");
    // TODO: put initial state to the output channel.
    // world parameters
    // object states
    glog.trace("Entering game cycle");
    // start wall-clock timer.
    hirez_timer wallclock; // governs the overall turn time
    hirez_timer turnclock; // measures the actual gonculation time.
    quanta=0;
    unsigned long long ticks = floor(quanta_duration * 1e6);
    unsigned long long nexttime = ticks;
    while (!world.end_run)
    {
      // reset turn timer.
      turnclock.reset();
      quanta++;
      turn_init(quanta);
      tick(quanta);
      collision_check();
      // resolve collisions
      // output turn status
      // -- output 
      // get turn elapsed
      long long elapsed = turnclock.interval_as_usec();
      // check for overrun
      if (elapsed >= ticks)
      {
	base_exception e;
        stringstream s;
        s << "Quanta " << quanta << " Overrun: " << elapsed << "usec";
	e.store(s.str());
        throw e;
      }; 
      // sleep until the next turn by the wall clock
      usleep(nexttime - wallclock.interval_as_usec());
      nexttime += ticks;
    };
  }
  // Catch most errors in our code..
  catch (base_exception &e)
  {
    glog.critical(e.what());
    glog.critical(e.comment());
    glog.critical("Run termimnated abnormally.");
  }
  // catch most errors in library code...
  catch (std::exception &e)
  {
    glog.critical(e.what());
    glog.critical("Run terminated abnormally.");
  }
  // unconditionally catch any other errors.
  catch (...)
  {
    glog.critical("Undefined exception");
    glog.critical("Run terminated abnormally.");
  };
  // close out nicely.
  glog.trace("complete");
  world.is_running = false;
};


