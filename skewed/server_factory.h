// A ServerFactory runs a server and listens for connections. Whenever a client
// connects, it calls build_protocol to get a Protocol and it runs that
// protocol to respond to the client's messages.
//
// The chunk_size is the max size of each message chunk sent to data_received.

#ifndef LIB_SKEWED_SERVER_FACTORY
#define LIB_SKEWED_SERVER_FACTORY

// For sockaddr_in.
#include <netinet/in.h>
// For free and memset, which we need to destroy malloc-ed buffers.
#include <stdlib.h>
// For sockaddr, sockaddr_storage, bind, listen, etc.
#include <sys/socket.h>
// For close, which we need to close sockets.
#include <unistd.h>

#include <algorithm>
#include <memory>
#include <set>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include "address.h"
#include "connection.h"
#include "macros.h"

namespace skewed {

class ServerFactory;

namespace {

const int kMinChunkSize = 1024;
const int kMaxChunkSize = 16384;

struct CallbackContext {
  ServerFactory* server;
  Connection* connection;
};

}  // namespace

class ServerFactory {
 public:
  // Override this to get a server that implements a new protocol.
  // This method should not take ownership of address.
  virtual Protocol* build_protocol(Address* address) { return new Protocol(); }

  // Blocks on success. Returns false on failure.
  bool run(int port);

  // Member function that corresponds to the libevent accept callback.
  void handle_accept(evconnlistener*, evutil_socket_t, sockaddr*, int);

  // Returns true if the given connection has not been closed yet.
  bool active(Connection* connection) {
    DCHECK_NOTNULL(connection);
    return !connection->closed() &&
        connections_.find(connection) != connections_.end();
  }

  ~ServerFactory() {
    if (listener_ != nullptr) evconnlistener_free(listener_);
    if (event_base_ != nullptr) event_base_free(event_base_);
  }

  // Creates a connection, registers in in the connection set, and returns it.
  Connection* register_connection(Address* address, bufferevent* bev) {
    Protocol* protocol = build_protocol(address);
    Connection* connection = new Connection(address, bev, protocol);
    connections_.insert(connection);
    return connection;
  }

  // Removes a connection from the connection set and destructs it.
  void drop_connection(Connection* connection) {
    DCHECK_NOTNULL(connection);
    DCHECK(connections_.find(connection) != connections_.end());
    connections_.erase(connection);
    connection->handle_close();
    delete connection;
  }

 private:
  std::set<Connection*> connections_;
  event_base* event_base_;
  evconnlistener* listener_;
};

namespace {

void accept_callback(
    evconnlistener* listener, evutil_socket_t fd, sockaddr* client_addr,
    int client_addr_size, void* context) {
  DCHECK_NOTNULL(context);
  ServerFactory* server = static_cast<ServerFactory*>(context);
  server->handle_accept(listener, fd, client_addr, client_addr_size);
}

void read_callback(bufferevent* bev, void* context) {
  DCHECK_NOTNULL(context);
  ServerFactory* server = static_cast<CallbackContext*>(context)->server;
  Connection* connection = static_cast<CallbackContext*>(context)->connection;
  DCHECK_NOTNULL(server);
  DCHECK_NOTNULL(connection);

  evbuffer* input = bufferevent_get_input(bev);
  DCHECK_NOTNULL(input);
 
  int length = evbuffer_get_length(input);
  if (length > 0) {
    int chunk_size = std::max(kMinChunkSize, std::min(kMaxChunkSize, length));
    char buffer[chunk_size];
    while (evbuffer_get_length(input)) {
      int bytes_read = evbuffer_remove(input, buffer, chunk_size);
      DCHECK(read > 0);
      std::string data(buffer, bytes_read);
      connection->handle_read(&data);
      if (connection->closed()) {
        server->drop_connection(connection);
        return;
      }
    }
  }
}

void error_callback(bufferevent* bev, short error, void* context) {
  // Use error & BEV_EVENT_x to determine which event type occurred.
  // EOF indicates that the client closed their socket, ERROR indicates a
  // transport layer error, and TIMEOUT is only issued if this was the
  // callback for a delay timer.
  DCHECK_NOTNULL(context);
  ServerFactory* server = static_cast<CallbackContext*>(context)->server;
  Connection* connection = static_cast<CallbackContext*>(context)->connection;
  DCHECK_NOTNULL(server);
  DCHECK_NOTNULL(connection);
  server->drop_connection(connection);
}

}  // namespace

void ServerFactory::handle_accept(
    evconnlistener* listener, evutil_socket_t fd,
    sockaddr* client_addr, int client_addr_size) {
  DCHECK_NOTNULL(listener);
  DCHECK_NOTNULL(client_addr);

  // Check that the file descriptor is valid. If not, abort.
  if (fd < 0 || fd > FD_SETSIZE) {
    DLOG("Got invalid socket file descriptor: %d", fd);
    close(fd);
    return;
  }

  // Retrieve an address for the newly connected client. If this fails, abort.
  std::unique_ptr<Address> address(new Address);
  int error;
  if (!address->set(client_addr, client_addr_size, &error)) {
    DLOG("Error setting client address: %d", error);
    close(fd);
    return;
  }

  // Set up the socket for this connection based on this server's parameters.
  evutil_make_socket_nonblocking(fd);
  event_base* base = evconnlistener_get_base(listener);
  DCHECK_NOTNULL(base);
  bufferevent* bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
  DCHECK_NOTNULL(bev);
  bufferevent_setwatermark(bev, EV_READ, 0, kMaxChunkSize);

  // Register the connection to the new client.
  Connection* connection = register_connection(address.release(), bev);
  CallbackContext* context = new CallbackContext{this, connection};
  bufferevent_setcb(bev, read_callback, NULL, error_callback, context);
  bufferevent_enable(bev, EV_READ|EV_WRITE);
}

bool ServerFactory::run(int port) {
  event_base_ = event_base_new();
  if (!event_base_) {
    LOG("Failed to initialize base!");
    return false;
  }

  sockaddr_in full_server_addr;
  memset(&full_server_addr, 0, sizeof(full_server_addr));
  full_server_addr.sin_family = AF_INET;
  full_server_addr.sin_addr.s_addr = 0;
  full_server_addr.sin_port = htons(port);
  sockaddr* server_addr = reinterpret_cast<sockaddr*>(&full_server_addr);

  listener_ = evconnlistener_new_bind(
      event_base_, accept_callback, this,
      LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
      -1, server_addr, sizeof(full_server_addr));
  if (!listener_) {
    LOG("Failed to create connection listener!");
    return false;
  }

  event_base_dispatch(event_base_);
  return true;
}

}  // namespace skewed

#endif  // LIB_SKEWED_SERVER_FACTORY
