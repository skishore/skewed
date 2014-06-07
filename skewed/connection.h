#ifndef LIB_SKEWED_CONNECTION
#define LIB_SKEWED_CONNECTION

#include <string>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include "macros.h"
#include "protocol.h"
#include "transport.h"

namespace skewed {

class Connection : Transport {
 public:
  void write(const std::string& data) override {
    evbuffer_add(output_, data.data(), data.size());
  }

  void lose_connection() override {
    closed_ = true;
  }

 private:
  Connection(bufferevent* bev, evutil_socket_t fd) : closed_(true) {
    DCHECK_NOTNULL(bev);
    output_ = bufferevent_get_output(bev);
    DCHECK_NOTNULL(output_);
    closed_ = false;
  }

  bool closed() const { return closed_; }

  bool closed_;
  evbuffer* output_;

  DISALLOW_COPY_AND_ASSIGN(Connection);
  friend class ServerFactory;
};

}  // namespace skewed

#endif  // LIB_SKEWED_CONNECTION
