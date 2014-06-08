// Defining a new protocol is the main way to use this library. A protocol
// responds to connection events, such as accept, recv, and close, and may
// write data out through the Transport interface.

#ifndef LIB_SKEWED_PROTOCOL
#define LIB_SKEWED_PROTOCOL

#include <string>

#include "macros.h"
#include "transport.h"

namespace skewed {

class Connection;

class Protocol {
 public:
  // Override this method or add new constructors as needed by your protocol.
  Protocol() : transport_(nullptr) {};

  // Override this method to respond to a client connection.
  virtual void connection_made() {
    DLOG("Connection made: %s", transport_->address().text().c_str());
  };

  // Override this method to respond to data the client sends.
  virtual void data_received(const std::string& data) {};

  // Override this method to respond to a client disconnect.
  virtual void connection_lost() {
    DLOG("Connection lost: %s", transport_->address().text().c_str());
  };

 protected:
  void make_connection(Transport* transport) {
    DCHECK_NOTNULL(transport);
    transport_ = transport;
    connection_made();
  }

  Transport* transport_;

  DISALLOW_COPY_AND_ASSIGN(Protocol);
  friend class Connection;
};

}  // namespace skewed

#endif  // LIB_SKEWED_PROTOCOL
