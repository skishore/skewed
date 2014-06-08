#ifndef LIB_SKEWED_CONNECTION
#define LIB_SKEWED_CONNECTION

#include <algorithm>
#include <string>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include "address.h"
#include "macros.h"
#include "protocol.h"
#include "transport.h"

namespace skewed {

namespace {

const int kMinChunkSize = 1024;
const int kMaxChunkSize = 16384;

}  // namespace

class Connection : Transport {
 public:
  // Implement the three methods needed for the Transport interface.

  const Address& address() const override { return *address_; }

  void write(const std::string& data) override {
    evbuffer_add(output_, data.data(), data.size());
  }

  void lose_connection() override { closed_ = true; }

 private:
  Connection(Address* address, bufferevent* bev, Protocol* protocol) :
      closed_(false), address_(address), protocol_(protocol), bev_(bev) {
    DCHECK_NOTNULL(address);
    DCHECK_NOTNULL(bev);
    DCHECK_NOTNULL(protocol);

    bufferevent_setwatermark(bev, EV_READ, 0, kMaxChunkSize);
    bufferevent_setcb(bev, handle_read, NULL, handle_error, this);
    output_ = bufferevent_get_output(bev);
    DCHECK_NOTNULL(output_);

    // We need to register ourselves as a transport for the protocol before
    // enabling events on bev, otherwise the protocol may get data before
    // connection_made is called.
    protocol_->make_connection(this);
    bufferevent_enable(bev, EV_READ|EV_WRITE);
  }

  // Static function that implements the libevent read event callback.
  static void handle_read(bufferevent* bev, void* context) {
    DCHECK_NOTNULL(context);
    Connection* connection = static_cast<Connection*>(context);
    DCHECK(connection->bev_ == bev);

    evbuffer* input = bufferevent_get_input(bev);
    DCHECK_NOTNULL(input);

    int length = evbuffer_get_length(input);
    if (length > 0) {
      int chunk_size = std::max(kMinChunkSize, std::min(kMaxChunkSize, length));
      char buffer[chunk_size];
      while (evbuffer_get_length(input)) {
        int bytes_read = evbuffer_remove(input, buffer, chunk_size);
        DCHECK(read > 0);
        std::string data(buffer, bytes_read);
        connection->protocol_->data_received(data);
        if (connection->closed_) {
          connection->protocol_->connection_lost();
          delete connection;
          return;
        }
      }
    }
  }

  // Static function that implements the libevent error event callback.
  static void handle_error(bufferevent* bev, short error, void* context) {
    // Use error & BEV_EVENT_x to determine which event type occurred.
    // EOF indicates that the client closed their socket, ERROR indicates a
    // transport layer error, and TIMEOUT is only issued if this was the
    // callback for a delay timer.
    DCHECK_NOTNULL(context);
    Connection* connection = static_cast<Connection*>(context);
    connection->protocol_->connection_lost();
    delete connection;
  }

  ~Connection() {
    // Also frees the and output buffer. (I hope... -skishore)
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
