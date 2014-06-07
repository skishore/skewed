// For sockaddr_in.
#include <netinet/in.h>
// For free, which we need to destroy malloc-ed buffers.
#include <stdlib.h>
// For sockaddr, sockaddr_storage, bind, listen, etc.
#include <sys/socket.h>
// For close, which we need to close sockets.
#include <unistd.h>

#include <memory>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include "skewed/connection.h"

#define MAX_LINE 16384
#define LOG(...) printf(__VA_ARGS__); printf("\n")

struct ClientContext {
  char host[NI_MAXHOST];
  char port[NI_MAXSERV];
};

char rot13_char(char c) {
  if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M')) {
    return c + 13;
  } else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z')) {
    return c - 13;
  } 
  return c;
}

void read_callback(bufferevent* bev, void* context) {
  evbuffer* input = bufferevent_get_input(bev);
  evbuffer* output = bufferevent_get_output(bev);

  char* line;
  size_t n;
  while ((line = evbuffer_readln(input, &n, EVBUFFER_EOL_LF))) {
    for (int i = 0; i < n; ++i) {
      line[i] = rot13_char(line[i]);
    }
    evbuffer_add(output, line, n);
    evbuffer_add(output, "\n", 1);
    free(line);
  }

  if (evbuffer_get_length(input) >= MAX_LINE) {
    // The remaining line input is too long. We process it in chunks.
    char buffer[1024];
    while (evbuffer_get_length(input)) {
      n = evbuffer_remove(input, buffer, sizeof(buffer));
      for (int i = 0; i < n; ++i) {
        buffer[i] = rot13_char(buffer[i]);
      }
      evbuffer_add(output, buffer, n);
    }
    evbuffer_add(output, "\n", 1);
  }
}

void error_callback(bufferevent* bev, short error, void* context) {
  // Use error & BEV_EVENT_x to determine which event type occurred.
  // EOF indicates that the client closed their socket, ERROR indicates a
  // transport layer error, and TIMEOUT is only issued if this was the
  // callback for a delay timer.
  ClientContext* client_context = static_cast<ClientContext*>(context);
  LOG("Lost: %s %s", client_context->host, client_context->port);
  bufferevent_free(bev);
  delete client_context;
}

void accept_callback(
    evconnlistener* listener, evutil_socket_t fd, sockaddr* client_addr,
    int client_addr_size, void* context) {
  if (fd < 0 || fd > FD_SETSIZE) {
    LOG("Got invalid socket file descriptor: %d", fd);
    close(fd);
    return;
  }

  ClientContext* client_context = new ClientContext;
  // Note that client_addr_size might be > sizeof(client_addr), because there
  // are address protocols with different lengths (ex. IPv4 vs. IPv6).
  int error = getnameinfo(
    client_addr, client_addr_size, client_context->host, NI_MAXHOST,
    client_context->port, NI_MAXSERV, NI_NUMERICHOST | NI_NUMERICSERV);
  if (error == 0) {
    LOG("Accepted: %s %s", client_context->host, client_context->port);
  } else {
    LOG("Cannot determine host address!");
    close(fd);
    return;
  }

  evutil_make_socket_nonblocking(fd);

  event_base* base = evconnlistener_get_base(listener);
  bufferevent* bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
  bufferevent_setcb(bev, read_callback, NULL, error_callback, client_context);
  bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
  bufferevent_enable(bev, EV_READ|EV_WRITE);
}

bool run() {
  event_base* base = event_base_new();
  if (!base) {
    LOG("Failed to initialize base!");
    return false;
  }

  sockaddr_in full_server_addr;
  std::memset(&full_server_addr, 0, sizeof(full_server_addr));
  full_server_addr.sin_family = AF_INET;
  full_server_addr.sin_addr.s_addr = 0;
  full_server_addr.sin_port = htons(1618);
  sockaddr* server_addr = reinterpret_cast<sockaddr*>(&full_server_addr);

  evconnlistener* listener = evconnlistener_new_bind(
      base, accept_callback, NULL, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
      -1, server_addr, sizeof(full_server_addr));
  if (!listener) {
    LOG("Failed to create connection listener!");
    return false;
  }

  event_base_dispatch(base);
  return true;
}

int main(int argc, char **argv) {
  setvbuf(stdout, NULL, _IONBF, 0);
  return (run() ? 0 : 1);
}
