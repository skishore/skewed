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
  // Overload these event callbacks to define a new protocol.
  virtual void connection_made() {};
  // data_received does not take ownership of the data, but it may mutate it.
  // data is guaranteed to not be NULL.
  virtual void data_received(std::string* data) {};
  virtual void connection_lost() {};

  Transport* transport_;

 private:
  void make_connection(Transport* transport) {
    DCHECK_NOTNULL(transport);
    transport_ = transport;
  }

  DISALLOW_COPY_AND_ASSIGN(Protocol);
};

}

#endif  // LIB_SKEWED_PROTOCOL
