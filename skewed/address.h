#ifndef LIB_SKEWED_ADDRESS
#define LIB_SKEWED_ADDRESS

#include <string>

#include <netdb.h>
#include "macros.h"

namespace skewed {

struct Address {
  // Returns false if it fails to parse the given socket address or if the
  // address was invalid. If so, this struct is not in a valid state.
  //
  // Does not take ownership of the passed pointers.
  bool set(const sockaddr& addr, int size, int* error) {
    DCHECK(error);
    *error = getnameinfo(
        &addr, size, host, NI_MAXHOST, port, NI_MAXSERV,
        NI_NUMERICHOST | NI_NUMERICSERV);
    return *error == 0;
  }

  std::string text() const {
    int max_size = NI_MAXHOST + NI_MAXSERV + 1;
    char buffer[max_size];
    int size = snprintf(buffer, max_size, "%s %s", host, port);
    return std::string(buffer, size);
  }

  char host[NI_MAXHOST];
  char port[NI_MAXSERV];
};

}  // namespace skewed

#endif  // LIB_SKEWED_ADDRESS
