#ifndef LIB_SKEWED_ADDRESS
#define LIB_SKEWED_ADDRESS

#include <string>

#include <netdb.h>
#include "macros.h"

namespace skewed {

struct Address {
  // Sets error to a nonzero value if it fails to parse the given socket
  // address. If so, this struct is not in a valid state.
  Address(const sockaddr& addr, int addr_size, int* error) {
    DCHECK_NOTNULL(error);
    *error = getnameinfo(
        &addr, addr_size, host, NI_MAXHOST, port, NI_MAXSERV,
        NI_NUMERICHOST | NI_NUMERICSERV);
  }

  const std::string text() const {
    int max_size = NI_MAXHOST + NI_MAXSERV + 1;
    char buffer[max_size];
    int size = snprintf(buffer, max_size, "%s %s", host, port);
    return std::string(buffer, size);
  }

  char host[NI_MAXHOST];
  char port[NI_MAXSERV];

  DISALLOW_COPY_AND_ASSIGN(Address);
};

}  // namespace skewed

#endif  // LIB_SKEWED_ADDRESS
