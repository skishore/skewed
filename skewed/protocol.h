// Defining a new protocol is the main way to use this library. A protocol
// responds to connection events, such as accept, recv, and close, and may
// write data out through the Transport interface.

#ifndef LIB_SKEWED_PROTOCOL
#define LIB_SKEWED_PROTOCOL

#include <string>

#include "macros.h"
#include "transport.h"

namespace skewed {

class Protocol {
 public:
  Protocol() : transport_(nullptr) {};

  // Overload these event callbacks to define a new protocol.
  virtual void connection_made() {
    DLOG("Connection made: %s %s",
        transport_->address().host, transport_->address().port);
  };
  // data_received does not take ownership of the data, but it may mutate it.
  // data is guaranteed to not be NULL.
  virtual void data_received(std::string* data) {};
  virtual void connection_lost() {
    DLOG("Connection lost: %s %s",
        transport_->address().host, transport_->address().port);
  };

  void make_connection(Transport* transport) {
    DCHECK_NOTNULL(transport);
    transport_ = transport;
  }

 protected:
  Transport* transport_;

  friend class Transport;
  DISALLOW_COPY_AND_ASSIGN(Protocol);
};

}  // namespace skewed

#endif  // LIB_SKEWED_PROTOCOL
