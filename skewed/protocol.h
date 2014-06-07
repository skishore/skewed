#ifndef LIB_SKEWED_PROTOCOL
#define LIB_SKEWED_PROTOCOL

#include <string>

#include "macros.h"
#include "transport.h"

namespace skewed {

class Protocol {
 public:
  virtual void connection_made() {};
  virtual void data_received(const std::string& data) {};
  virtual void connection_lost() {};

 private:
  void make_connection(Transport* transport) {
    CHECK_NOTNULL(transport);
    transport_ = transport;
  }

  Transport* transport_;

  DISALLOW_COPY_AND_ASSIGN(Protocol);
};

}

#endif  // LIB_SKEWED_PROTOCOL
