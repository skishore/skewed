// Transport is simply an interface that allows writes to a socket.
//
// This interface is implemented by the Connection object, which also maintains
// the connection status and takes care of cleanup when it is closed.

#ifndef LIB_SKEWED_TRANSPORT
#define LIB_SKEWED_TRANSPORT

#include <string>

namespace skewed {

class Transport {
 public:
  virtual void write(const std::string& message) = 0;
  virtual void lose_connection() = 0;
};

}  // namespace skewed

#endif  // LIB_SKEWED_TRANSPORT
