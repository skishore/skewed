#ifndef LIB_SKEWED_ADDRESS
#define LIB_SKEWED_ADDRESS

#include <netdb.h>
#include "macros.h"

namespace skewed {

struct Address {
  // Returns false if it fails to parse the given socket address or if the
  // address was invalid. If so, this struct is not in a valid state.
  //
  // Does not take ownership of the passed pointers.
  bool set(sockaddr* addr, int size, int* error) {
    DCHECK(error);
    *error = getnameinfo(
        addr, size, host, NI_MAXHOST, port, NI_MAXSERV,
        NI_NUMERICHOST | NI_NUMERICSERV);
    return *error == 0;
  }

  char host[NI_MAXHOST];
  char port[NI_MAXSERV];
};

}  // namespace skewed

#endif  // LIB_SKEWED_ADDRESS
