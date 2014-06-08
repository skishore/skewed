#ifndef LIB_SKEWED_CONNECTION
#define LIB_SKEWED_CONNECTION

#include <string>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include "address.h"
#include "macros.h"
#include "protocol.h"
#include "transport.h"

namespace skewed {

class Connection : Transport {
 public:
  // These three methods are needed to implement the Transport interface.
  const Address& address() const override { return *address_; }

  void write(const std::string& data) override {
    evbuffer_add(output_, data.data(), data.size());
  }

  void lose_connection() override { closed_ = true; }

  void handle_read(const std::string& data) { protocol_->data_received(data); }

  void handle_close() { protocol_->connection_lost(); }

  bool closed() const { return closed_; }

 private:
  Connection(Address* address, bufferevent* bev, Protocol* protocol) :
      closed_(true), address_(address), protocol_(protocol), bev_(bev) {
    DCHECK_NOTNULL(address);
    DCHECK_NOTNULL(bev);
    DCHECK_NOTNULL(protocol);

    output_ = bufferevent_get_output(bev);
    DCHECK_NOTNULL(output_);
    closed_ = false;

    protocol_->make_connection(this);
    protocol_->connection_made();
  }

  ~Connection() {
    // Also frees the output buffer. (I hope... -skishore)
    bufferevent_free(bev_);
  }

  bool closed_;
  std::unique_ptr<Address> address_;
  std::unique_ptr<Protocol> protocol_;

  bufferevent* bev_;
  evbuffer* output_;

  DISALLOW_COPY_AND_ASSIGN(Connection);
  friend class ServerFactory;
};

}  // namespace skewed

#endif  // LIB_SKEWED_CONNECTION
