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
 
#ifndef base_socket_header
#define base_socket_header

#include <memory>
#include <atomic>
#include <thread>
#include <common.h>
#include <linear_queue.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace nrtb
{

/** Provides an easy to use TCP/IP bidirectional communciations socket.
 ** 
 ** tcp_socket contains all the functionality you'll need for 98% of 
 ** your normal, non-encrypted networking communications needs. Everything
 ** from creating the socket, binding it if needed, name resolution, 
 ** connecting, sending and receiving, etc. is handled here.
 ** 
 ** Unless you override it, a tcp_socket closes automatically when it
 ** goes out of scope, preventing hanging sockets. 
 **/
class tcp_socket
{
public:

  enum class state {sock_undef,sock_init,sock_connect,sock_close};
  
protected:

  int mysock {0};
  bool close_on_destruct {true};
  state _status {state::sock_undef};
  int _last_error {0};
  std::vector<unsigned char> inbuff;

  virtual int transmit(const std::string & data);
  virtual int receive(std::string & data,int limit);

public:

  /// Use to catch all socket exceptions.
  class general_exception: public base_exception {};
  /// Thrown by send or get* if the socket is not open for use.
  class not_open_exception: public general_exception {};
  /// Thrown by get* if more than maxlen chars are receieved.
  class overrun_exception: public general_exception {};
  /// Thrown if a timeout occured during an operation.
  class timeout_exception: public general_exception {};
  /// Thrown if the socket could not connect to the target.
  class bad_connect_exception: public general_exception {};
  /// Thrown if the address argument is not interpretable.
  class bad_address_exception: public general_exception {};
  /// Thrown by bind() if the address/port is not bindable.
  class cant_bind_exception: public general_exception {};
  /// Thrown by close() if an error is reported.
  class close_exception: public general_exception {};
  /// Thrown by send if the message is too large.
  class msg_too_large_exception: public general_exception {};
  /// Thrown by send if buffer overflows.
  class buffer_full_exception: public general_exception {};
  /// Thrown if the socket lib complains of bad args.
  class bad_args_exception: public general_exception {};

  /** Constructs a tcp_socket.
    ** 
    ** Prepares this socket for use. If autoclose=true the socket will be
    ** closed when the object is distructed, otherwise it will be left in
    ** whatever state it happened to be in at the time.
    ** 
    ** BTW, I _don't_ recommend you leave the socket open. If the tcp_socket
    ** was instanciated via this constructor, there will not be any other 
    ** references to the socket save the one held privately, so a socket left
    ** open will be there forever. This will almost certainly warrent an 
    ** unpleasent visit by one or more SAs of the box(es) hosting your
    ** application over time. 
    ** 
    ** autoclose=true; it's the only way to be sure. ;)
    **/
  tcp_socket(bool autoclose=true);

  /** Constructs from an already existing socket.
    ** 
    ** Constructs the tcp_socket from an already existing socket.  This 
    ** constructor is specifically for use with the traditional listen()
    ** sockets function, and probably should not be used in other contexts.
    ** 
    ** If autoclose is true the socket will be closed on destruction of the 
    ** object, otherwise the socket will be left in whatever state it happened
    ** to be in at the time. As a general rule I suspect you'll want leave 
    ** autoclose defaulted to true.
    ** 
    ** In most cases where you would use listen(), take a look at the 
    ** tcp_server_socket_factory class; it'll probably do everything you need
    ** with a minimum of programming overhead.
    ** 
    ** BTW, I _don't_ recommend you leave the socket open. If the socket variable
    ** used for instanciation was discarded, there will not be any other 
    ** references to the socket save the one held privately, so a socket left
    ** open will be there forever. This will almost certainly warrent an 
    ** unpleasent visit by one or more SAs of the box(es) hosting your
    ** application over time. 
    ** 
    ** autoclose=true; it's the only way to be sure. ;)
    **/
  tcp_socket(int existing_socket, bool autoclose=true);

  /** Destructs a tcp_socket.
    ** 
    ** If autoclose was set to true, the socket will be closed if it is open
    ** at the time of destruction. If no, the socket will be left in whatever
    ** state it happened to be in at the time.
    **/
  virtual ~tcp_socket();

  /** Resets the socket after a hard error.
    **
    ** In cases where the socket was invlidated by the operating system
    ** (typically cases where a SIGPIPE was raised by the TCP/IP stack),
    ** use this function to discard the old socket id and aquire a new one.
    ** Understand that this will make the tcp_socket useful again, but you
    ** will have to reconnect before any traffic can be sent.
    **/
  virtual void reset();

  /** Creates a sockaddr_in from a formatted string.
    ** 
    ** address is an "IP:port" formatted string; i.e. 129.168.1.1:80. 
    ** "*" may use used to substatute for the IP, port or both.
    ** 
    ** Returns a properly formatted sockaddr_in struct for use with connect,
    ** bind, etc. Throws a bad_address_exception if the address argument is 
    ** mal-formed.
    **/
  static sockaddr_in str_to_sockaddr(const std::string & address);

  /** Creates a string from a sockaddr_in.
    ** 
    ** The returned string is of the same format accepted as arguments to the 
    ** to the bind() and connect() methods.
    ** 
    ** May throw a bad_address_exception if the sock_addr_in is not 
    ** properly formatted.
    **/
  static std::string sockaddr_to_str(const sockaddr_in & address);

  /** binds the tcp_socket to a local address/port.
    ** 
    ** You do _not_ need to use this method if you are setting up a tcp_socket
    ** for client use. If you don't call bind  an ephemeral port and the host's 
    ** IP number will be assigned, as is normal and preferred for client sockets.
    ** 
    ** In very rare cases where you have a multi-homed host and a real
    ** requirement to specify which IP this connection uses as local, you can
    ** use this method to select the port and address the client will use.
    ** 
    ** address: string of the format IP:port; example "255.255.255.255:91". 
    ** If you wish to accept input on any active IP address in this host, 
    ** use "*" for the IP address, as in "*:80". If you want to fix the address
    ** but leave the port up to the host, use "*" for the port, as in 
    ** "10.101.78.33:*". Obviously, using "*:*" as the address gives the 
    ** same results as not calling this method at all.
    ** 
    ** If the address argument is mal-formed a bad_address_exception will be
    ** thrown and no changes will be made to the tcp_socket.
    ** 
    ** If the address is not bindable (already in use is a common cause) a 
    ** cant_bind_exception will be thrown.
    **/
  void bind(const std::string & address);

  /** Opens the connection.
    **
    ** address: string of the format IP:port; example "255.255.255.255:91". 
    ** If the address is mal-formed a bad_address_exception will be thrown.
    ** 
    ** If timeout is specified and >0, the system will not wait more than
    ** timeout seconds before throwing a timeout_exception.
    ** 
    ** If the connection is not set up properly, a bad_connection_exception
    ** will be thrown.
    **/
  void connect(const std::string & address, int timeout=0);

  /** Closes an open connection.
    ** 
    ** If a problem is reported a close_exception will be thrown. Possible
    ** causes include the socket not being open, unsent data pending in the 
    ** tcp stack, etc.
    **/
  virtual void close();
  
  /** Returns the current status.
    ** 
    ** Possibles states are tcp_socket::undef, tcp_socket::init, 
    ** tcp_socket::connect, and tcp_socket::close.
    **/
  state status() { return _status; };

  /** Returns the last socket error recieved.
    ** 
    ** Will contain zero if no errors have occured on the socket since 
    ** the tcp_socket was instanciated, or the errno returned on the most recent
    ** error otherwise.
    ** 
    ** Check the man pages for bind(), connect(), close(), socket(), send(),
    ** and recv() for possible values. They are defined in error.h.
    **/
  int last_error() { return _last_error; };

  /** Sends the string to the target host.
    ** 
    ** If timeout is specified and >0, a timeout_exception will be thrown 
    ** if the send does not succeed within timeout seconds. 
    ** 
    ** If socket is not connected or closes during sending a 
    ** not_open_exception will be thrown. 
    ** 
    ** If an exception is thrown, the characters that were not
    ** sent will be available via the exception's comment() method (as
    ** a string, of course).
    **/
  int put(std::string s, int timeout=10);

  /** gets a string of characters from the socket.
    ** 
    ** This method allows the reception of a string of characters 
    ** limited by length and time. Use this method when you are expecting a 
    ** stream if data that you either know the length of or where there is
    ** no specific ending character.  If the expected traffic is organized 
    ** into messages or lines ending with a specific character or string, 
    ** use the getln() method instead.
    ** 
    ** Examples:
    ** 
    ** get() or get(1) will return a string containing a single
    ** character or throw a timeout in 10 seconds if one is not
    ** recieved.
    ** 
    ** get(100,30) will return when 100 characters have been recieved or 
    ** throw a timeout in 30 seconds.
    ** 
    ** get(100,-1) will return after 100 characters have been recieved, and will 
    ** _never_ time out. I strongly recommend that at the very least use timeout
    ** values >= 0 to this method to prevent program long term program stalls.
    ** 
    ** If maxlen==0, this method will return immediately with whatever characters
    ** happen to be in the socket's input buffer at the time.
    ** 
    ** If maxlen>0 and timeout<0, this method will block until maxlen characters
    ** have been recieved and then return. NOTE: this method could block 
    ** forever if the sending host does not send maxlen characters and timeout
    ** is < 0.
    ** 
    ** If maxlen>0 and timeout>0 this method will return upon recieving 
    ** maxlen characters or throw a timeout_exception after timeout seconds 
    ** have expired in the worse case.
    ** 
    ** In all cases the method returns a string containing the characters 
    ** recieved, if any.
    ** 
    ** If the timeout expires before the required number of characters are
    ** recieved a timeout_exception will be thrown. 
    ** 
    ** If the socket is not connected or closes while being read a 
    ** not_open_exception will be thrown.
    ** 
    ** If an exception is thrown, any characters already recieved 
    ** will be available via the exception's comment() method.
    **/
  std::string get(int maxlen=1, int timeout=10);

  /** gets a string of characters from the socket.
    ** 
    ** This method is for the reception of messages (or lines) that have a 
    ** specific ending string. As such it considers the recept of maxlen
    ** characters or a timeout without the designated eol string to be 
    ** a problem and will throw an exception in those cases. This behavor 
    ** differs from that of the get() method, which simply returns when it 
    ** has received its limit.
    ** 
    ** If there is no specific eol or eom string in your expected data 
    ** stream use the get() method instead.
    ** 
    ** Examples:
    ** 
    ** getln() will return all characters recieved up to and including the 
    ** first carrage return or throw a timeout if a carrage return is not
    ** recieved after 10 seconds.
    ** 
    ** getln("</alert>") will return all characters recieved up to and
    ** including "</alert>" or throw a timeout if "</alert>" is not recieved 
    ** within 10 seconds.
    ** 
    ** getln(":",30) will return all characters recieved up to and including 
    ** ":" if ":" is recieved by the 30th character. If 30 characters are recieved 
    ** before ":", an overrun_exception would be thrown. A timeout would be
    ** thrown if ":" is not recieved within 10 seconds.
    ** 
    ** getln(":",30,20) would react exactly like getln(":",30) except the timeout
    ** period would be 20 seconds instead of the default 10.
    ** 
    ** getln(":",30,-1) will react exactly like getln(":",30) except that no
    ** timeout will ever be thrown. In this case, the method may block 
    ** forever if no ":" is received and it never receives 30 characters.
    ** I strongly recommend that at the very least that timeout be a 
    ** value >= 0 to this method to prevent program long term program stalls.
    ** 
    ** If maxlen==0 and timeout<0, this method will block until the eol
    ** string is recieved from the host or maxlen characters
    ** have been recieved. and then return. NOTE: this method could block 
    ** forever if the sending host does not send maxlen characters or the
    ** eol string and timeout is < 0;
    ** 
    ** If maxlen>0 and timeout>0 this method will return upon recieving the 
    ** eol string, when maxlen characters have be recieved or when 
    ** timeout seconds have expired.
    ** 
    ** In all cases the method returns a string containing the characters 
    ** recieved, if any, including the eol string.
    ** 
    ** If the timeout expires before eol is recieved or maxlen characters are
    ** recieved a timeout_exception will be thrown.
    ** 
    ** If maxlen characters are recieved before the eol string is recieved 
    ** an overrun_exception will be thown.
    ** 
    ** If the socket is not connected or closes while being read a 
    ** not_open_exception will be thrown.
    ** 
    ** If an exception is thrown, any characters already recieved 
    ** will be available via the exception's comment() method.
    **/
  std::string getln(std::string eol="\r", int maxlen=0, int timeout=10);

  /** Returns the local address.
    ** 
    ** Returns the address in the same form that it needs to be in for the 
    ** bind and connect addess arguments.
    ** 
    ** May throw a buffer_full_exception if there are not enough resources, 
    ** or a general_exception if some other problem occurs.
    **/
  std::string get_local_address();

  /** Returns the remote address.
    ** 
    ** Returns the address in the same form that it needs to be in for the 
    ** bind and connect addess arguments.
    ** 
    ** May throw a buffer_full_exception if there are not enough resources,
    ** a not_open_exception if the socket has not been connected to a remote
    ** host, or a general_exception if some other problem occurs.
    **/
  std::string get_remote_address();
};

/// smart pointer for use with tcp_sockets
typedef std::unique_ptr<nrtb::tcp_socket> tcp_socket_p;

/** "listener" TCP/IP socket socket factory for servers. 
 ** 
 ** Upon construction this class establishes a listening socket
 ** on the specified address and port. Each incomming connection
 ** is automatically accepted and its socket put on queue for other 
 ** tasks to process.  
 **/
class tcp_server_socket_factory
{
public:

  /// Use to catch all server_socket_factory exceptions.
  class general_exception: public base_exception {};
  /// Thrown if start_listen is called while already listening
  class already_running_exception: public general_exception {};
  /// Thrown if we can not allocate a new connect_sock* as needed.
  class mem_exhasted_exception: public general_exception {};
  /// Thrown by start_listen() if the IP/port could not be bound.
  class bind_failure_exception: public general_exception {};
  /// Thrown by by the listen thread in case of unexpected error.
  class listen_terminated_exception: public general_exception {};
  /// handlers should catch this and shutdown gracefully.
  typedef linear_queue<tcp_socket_p>::queue_not_ready queue_not_ready;
  
  /** Construct a tcp_server_socket_factory and puts it online.
    ** 
    ** address: std::string of the format IP:port; example "255.255.255.255:91". 
    ** if you wish to accept input on any active IP address in this host, 
    ** use "*" for the IP address, as in "*:80". The port number is 
    ** required.
    ** 
    ** backlog: The number of backlog connections (connections pending accept)
    ** allowed for this socket. The limit is operating system dependent. If
    ** the supplied value is 0 or less, the default value for the operating 
    ** system will be used.
    ** 
    ** queue_size: Optional; the size of the queue for connections
    ** waiting to be serviced. Defaults to 10. If queue_size is 
    ** exceeded, oldest connections will be discarded. 
    **/
  tcp_server_socket_factory(const std::string & address, 
	const unsigned short int backlog = 5);

  /** Destructs a server_sock. 
    ** 
    ** If the server_sock is currently listening the listener thread is 
    ** shut down and the socket released immediately, rudely if necessary.
    ** 
    ** It's preferable to call stop_listen() before desconstruction to allow 
    ** graceful termination of the listener thread and teardown of the port.
    **/
  virtual ~tcp_server_socket_factory();

  /// Consumers call this to get a connected socket.
  tcp_socket_p get_sock();
  
  /// returns the number of connections received.
  int accepted() { return pending.in_count; };
  /// returns the number of connections consumed.
  int processed() { return pending.out_count; };
  /// returns the number of connections waiting to be consumed.
  int available() { return pending.size(); };
  /// returns the number of connections dropped due to overflow.
  int discarded() 
  { 
    return pending.in_count - 
	pending.out_count - pending.size(); 
  };

  /** Stop listening for inbound connections.
    ** 
    ** This shuts down the listener thread and tears down the TCP/IP port 
    ** before returning. An exception will be thrown if a problem occurs.
    ** 
    ** If a connection is being actively processed (defined as thread 
    ** execution being post accept() and prior to the return of on_accept()),
    ** the return will be delayed until on_accept() is complete, though 
    ** the listening socket will be torn down immediately. If on_accept()
    ** fails to return within 30 seconds, the listener thread will be
    ** rudely murdered and an on_accept_bound_exception will be thrown. In the
    ** worse case it may be that the listener thread will not die for some time,
    ** though it should be cleanly cancelable at most any time.
    ** 
    ** If the listener thread is not running this method returns immediately
    ** without taking any actions.
    ** 
    ** Call this method when your application wishes to stop recieving input, 
    ** even if you are going to restart reception later. 
    **/
  void stop_listen();

  /** Starts a stopped listener
   **/
  void start_listen();

  /** Monitors listening status.
    ** 	
    ** At it's simplist, this method returns true if the socket is listening 
    ** for new connections, and false if if it not. Additionally, this 
    ** method will throw a listen_terminated_exception if the listener thread
    ** died unexpectedly. Therefore, calling this method periodically is an
    ** easy way to monitor the health of the listener thread.
    **/
  bool listening();

  /** Returns the last listen thread fault code.
    ** 
    ** Will be 0 if the listener has never faulted. If -1, an untrapped 
    ** exception was caught (likely thrown by on_accept()); if -2 the thread 
    ** did not manage to initialize the listening socket successfully, and  
    ** if > 0 then it'll be the error code of the socket error caught that 
    ** forced the thread shutdown.
    ** 
    ** check this if you catch a listen_terminated_exception to find out
    ** what really happened.
    **/
    int last_fault();

  /** Returns the max number of backlog connections.
    **/
  unsigned short int backlog();

private:

  // the address:port the listening socket will connect to.
  std::string _address;
  unsigned short int _backlog;
  // stuff for the listener thread
  std::thread work_thread;
  std::shared_ptr<tcp_socket> listen_sock {nullptr};
  std::atomic< int > _last_thread_fault {0};
  std::atomic< bool > in_run_method {false};
  // The accepted inbound connection queue
  nrtb::linear_queue<tcp_socket_p> pending;
  // Provides the listener thread.
  static void run(tcp_server_socket_factory * server);
  
};

} // namepace nrtb

#endif // base_socket_header
