#include <string>
#include <skewed/transport.h>

namespace skewed {

class Protocol {
 public:
  virtual void connection_made() = 0;
  virtual void connection_lost() = 0;
  virtual void line_received(const std::string& line) = 0;

 protected:
  Transport transport_;
};

}
