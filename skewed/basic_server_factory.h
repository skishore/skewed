// If your protocol has a default constructor, you can use BasicServerFactory:
//   class MyProtocol : Protocol { ... }
//   BasicServerFactory<MyProtocol> server;
//   server.run(8000);

#ifndef LIB_SKEWED_BASIC_SERVER_FACTORY
#define LIB_SKEWED_BASIC_SERVER_FACTORY

#include "protocol.h"
#include "server_factory.h"

namespace skewed {

template <class P>
class BasicServerFactory : public ServerFactory {
 public:
  Protocol* build_protocol(const Address& address) override {
    return new P();
  }
};

}  // namespace skewed

#endif  // LIB_SKEWED_BASIC_SERVER_FACTORY
