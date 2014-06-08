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

#include <memory>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include "address.h"
#include "connection.h"
#include "macros.h"

namespace skewed {

class ServerFactory {
 public:
  // Override this to get a server that implements a new protocol.
  // This method should not take ownership of address.
  virtual Protocol* build_protocol(const Address& address) {
    return new Protocol();
  }

  // Blocks on success. Returns false on failure.
  //
  // TODO(skishore): This method leaks memory. Specifically, if this server
  // is somehow stopped while it still has connections open, those connections
  // will never be cleaned up. This doesn't matter much at the moment, because
  // no binary that calls run does anything afterwards but run the server...
  bool run(int port) {
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
        event_base_, handle_accept, this,
        LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
        -1 /* let libevent set backlog */,
        server_addr, sizeof(full_server_addr));
    if (!listener_) {
      LOG("Failed to create connection listener!");
      return false;
    }

    event_base_dispatch(event_base_);
    return true;
  }

  ~ServerFactory() {
    if (listener_ != nullptr) evconnlistener_free(listener_);
    if (event_base_ != nullptr) event_base_free(event_base_);
  }

 private:
  // Static function that implements the libevent accept event callback.
  static void handle_accept(
      evconnlistener* listener, evutil_socket_t fd, sockaddr* client_addr,
      int client_addr_size, void* context) {
    DCHECK_NOTNULL(listener);
    DCHECK_NOTNULL(client_addr);
    DCHECK_NOTNULL(context);
    ServerFactory* server_factory = static_cast<ServerFactory*>(context);

    // Check that the file descriptor is valid. If not, abort.
    if (fd < 0 || fd > FD_SETSIZE) {
      DLOG("Got invalid socket file descriptor: %d", fd);
      close(fd);
      return;
    }

    // Retrieve an address for the newly connected client. If this fails, abort.
    int error;
    std::unique_ptr<Address> address(
        new Address(*client_addr, client_addr_size, &error));
    if (error != 0) {
      DLOG("Error setting client address: %d", error);
      close(fd);
      return;
    }

    // Configure the socket and retrieve the bufferevent event wrapper for it.
    evutil_make_socket_nonblocking(fd);
    event_base* base = evconnlistener_get_base(listener);
    DCHECK_NOTNULL(base);
    bufferevent* bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);

    // Create the new Connection object. This object will be owned by the read
    // and error callbacks it registers - kind of strange, but it works.
    //
    // The connection will be passed as the void* context for these callbacks.
    // When the error callback is called, the connection will be deleted.
    Protocol* protocol = server_factory->build_protocol(*address);
    new Connection(address.release(), bev, protocol);
  }

  event_base* event_base_;
  evconnlistener* listener_;
};

}  // namespace skewed

#endif  // LIB_SKEWED_SERVER_FACTORY
