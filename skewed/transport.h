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
  virtual const Address& address() const = 0;

  virtual void write(const std::string& message) = 0;
  // lose_connection must be called within one of a Protocol's callbacks,
  // otherwise its behavior is undefined. In addition, the behavior of write
  // after a call to lose_connection is undefined.
  virtual void lose_connection() = 0;

 protected:
  virtual ~Transport() {};
};

}  // namespace skewed

#endif  // LIB_SKEWED_TRANSPORT
