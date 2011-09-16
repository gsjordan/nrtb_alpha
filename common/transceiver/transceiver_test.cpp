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
 
#include <string>
#include <iostream>
#include <base_thread.h>
#include <base_socket.h>
#include "transceiver.h"
#include <log_setup.h>
#include <sim_to_db_wrapper.pb.h>
#include <boost/random.hpp>

using namespace nrtb;
using namespace std;

typedef nrtb_msg::sim_to_db my_msg;
typedef transceiver<my_msg,my_msg> linkt;

class safe_counter
{
private:
  mutex data_lock;
  int er_count;

public:
  
  safe_counter() { er_count = 0; };

  ~safe_counter() {};

  void inc()
  {
	scope_lock lock(data_lock);
	er_count++;
  };

  int operator ()()
  {
	scope_lock lock(data_lock);
	return er_count;
  };
};

safe_counter er_count;

class server_work_thread: public thread
{
public:
  
  tcp_socket_p sock;
  unsigned long long last_inbound;
  
  ~server_work_thread()
  {
	cout << "Destructing server_work_thread" << endl;
	sock.reset();
  };
  
  void run()
  {
	set_cancel_anytime();
	linkt link(sock);
	while (sock->status() == tcp_socket::sock_connect)
	{
	  try 
	  {
		linkt::out_ptr inbound = link.get();
		last_inbound = inbound->msg_uid();
		cout << "\tReceived #" << last_inbound << endl;
		link.send(inbound);
		if (last_inbound == 99)
		{
		  cout << "Receiver thread closing." << endl;
		  exit(0);
		};
	  }
	  catch (linkt::general_exception & e)
	  {
		cerr << "Server work thread caught " << e.what()
		  << "\n\tComment: " << e.comment() << endl;
		er_count.inc();;
	  }
	  catch (tcp_socket::general_exception & e)
	  {
		cerr << "Server work thread caught " << e.what()
		  << "\n\tComment: " << e.comment() << endl;
		er_count.inc();;
	  }
	  catch (std::exception & e)
	  {
		cerr << "Server work thread caught " << e.what() 
		  << endl;
		er_count.inc();;
	  };
	};
  };
};

class listener: public tcp_server_socket_factory
{
protected:
  boost::shared_ptr<server_work_thread> task;

public:
  listener(const string & add, const int & back)
   : tcp_server_socket_factory(add, back) {};
  ~listener()
  {
	cout << "Destructing listener" << endl;
	task.reset();
  };
  
  void on_accept()
  {
	if (!task)
	{
	  task.reset(new server_work_thread);
	  task->last_inbound = 0;
	  task->sock = connect_sock;
	  task->start(*(task.get()));
	  cout << "server thread running." << endl;
	}
	else
	{
	  connect_sock->close();
	  cerr << "Multiple attempts to connect to server" 
		<< endl;
	};
  };
};

string address = "127.0.0.1:";
int port_base = 12334;

int main()
{
  setup_global_logging("transceiver.log");

  try
  {
	//set up our port and address
	boost::mt19937 rng;
	rng.seed(time(0));
	boost::uniform_int<> r(0,1000);
	stringstream s;
	s << address << port_base + r(rng);
	address = s.str();
	cout << "Using " << address << endl;

	// kick off the listener thread.
	listener server(address,5);
	server.start_listen();
	usleep(1e4);

	// set up our sender
	tcp_socket_p sock(new tcp_socket);
	sock->connect(address);
	linkt sender(sock);

	// Let's send a few things.
	for (int i=0; i<100; i++)
	{
	  linkt::out_ptr msg(new my_msg);
	  sender.send(msg);
	  cout << "Sent " << msg->msg_uid() << endl;
	  msg = sender.get();
	  cout << "Got back " << msg->msg_uid() << endl;
	};
	usleep(1e4);
  }
  catch (...)
  {
	cout << "exception caught during test." << endl;
	er_count.inc();
  };

  int faults = er_count(); 
  if (faults)
  {
	cout << "========== ** There were " << faults 
	  << "errors logged. =========" << endl; 
  }
  else
	cout << "========= nrtb::transceiver test complete.=========" 
	  << endl;

  return faults;
};