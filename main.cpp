#include "skewed/address.h"
#include "skewed/protocol.h"
#include "skewed/server_factory.h"

class Rot13Protocol : public skewed::Protocol {
 public:
  void data_received(const std::string& data) override {
    std::string result(data);
    for (int i = 0; i < result.size(); i++) {
      result[i] = rot13_char(result[i]);
    }
    transport_->write(result);
  }

  static char rot13_char(char c) {
    if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M')) {
      return c + 13;
    } else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z')) {
      return c - 13;
    }
    return c;
  }
};

class Rot13Server : public skewed::ServerFactory {
  virtual skewed::Protocol* build_protocol(
      const skewed::Address& address) override {
    return new Rot13Protocol();
  }
};

int main(int argc, char **argv) {
  setvbuf(stdout, NULL, _IONBF, 0);
  Rot13Server server;
  return (server.run(1618) ? 0 : 1);
}
