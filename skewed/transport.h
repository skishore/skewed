// The Transport interface allows the user to write to or close a connection.
//
// This interface is implemented by the Connection object, which also maintains
// the connection status and takes care of cleanup when it is closed.

#ifndef LIB_SKEWED_TRANSPORT
#define LIB_SKEWED_TRANSPORT

#include <string>

#include "address.h"

namespace skewed {

class Transport {
 public:
  // Get the address of the client to which this transport is linked.
  virtual const Address& address() const = 0;

  // Write the given message out to the client.
  virtual void write(const std::string& message) = 0;

  // lose_connection must be called within one of a Protocol's callbacks,
  // otherwise its behavior is undefined. In addition, the behavior of write
  // after a call to lose_connection is undefined.
  //
  // TODO(skishore): We could enforce these requirements. Doesn't seem needed.
  virtual void lose_connection() = 0;

 protected:
  virtual ~Transport() {};
};

}  // namespace skewed

#endif  // LIB_SKEWED_TRANSPORT
