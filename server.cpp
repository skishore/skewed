// For sockaddr_in.
#include <netinet/in.h>
// For free, which we need to destroy malloc-ed buffers.
#include <stdlib.h>
// For sockaddr, sockaddr_storage, bind, listen, etc.
#include <sys/socket.h>
// For close, which we need to close sockets.
#include <unistd.h>

// The actual imports of the libevent library.
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <memory>

#define MAX_LINE 16384
#define LOG(...) printf(__VA_ARGS__); printf("\n")

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

void event_callback(bufferevent* bev, short error, void* context) {
  // User error & BEV_EVENT_x to determine which event type occurred.
  // EOF indicates that the client closed their socket, ERROR indicates a
  // transport layer error, and TIMEOUT is only issued if this was the
  // callback for a delay timer.
  bufferevent_free(bev);
}

void accept_callback(evutil_socket_t listener, short event, void *arg) {
  event_base* base = static_cast<event_base*>(arg);

  sockaddr_storage full_client_addr;
  socklen_t client_size = sizeof(full_client_addr);
  sockaddr* client_addr = reinterpret_cast<sockaddr*>(&full_client_addr);

  int fd = accept(listener, client_addr, &client_size);

  if (fd < 0) {
    LOG("Got invalid fd: %d", fd);
  } else if (fd > FD_SETSIZE) {
    close(fd);
  } else {
    char host[NI_MAXHOST];
    char port[NI_MAXSERV];
    int rc = getnameinfo(
      client_addr, client_size, host, sizeof(host), port, sizeof(port),
      NI_NUMERICHOST | NI_NUMERICSERV);
    if (rc == 0) {
      LOG("Accepted connection from %s:%s", host, port);
    } else {
      LOG("Accepted connection from unknown host!");
    }

    evutil_make_socket_nonblocking(fd);
    bufferevent* bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, read_callback, NULL, event_callback, NULL);
    bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
    bufferevent_enable(bev, EV_READ|EV_WRITE);
  }
}

bool run(void) {
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

  evutil_socket_t listener = socket(AF_INET, SOCK_STREAM, 0);
  evutil_make_socket_nonblocking(listener);

#ifndef WIN32
  {
    int one = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  }
#endif

  if (bind(listener, server_addr, sizeof(full_server_addr)) < 0) {
    LOG("Failed to bind socket!");
    return false;
  }

  if (listen(listener, 16) < 0) {
    LOG("Failed to listen on socket!");
    return false;
  }

  event* listener_event = event_new(
      base, listener, EV_READ|EV_PERSIST, accept_callback, (void*)base);
  if (!listener_event) {
    LOG("Failed to create listener_event!");
    return false;
  }

  event_add(listener_event, NULL);
  event_base_dispatch(base);
  return true;
}

int main(int argc, char **argv) {
  setvbuf(stdout, NULL, _IONBF, 0);
  return (run() ? 0 : 1);
}
