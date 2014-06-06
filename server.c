/* For sockaddr_in */
#include <netinet/in.h>
/* For socket functions */
#include <sys/socket.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

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

void read_callback(struct bufferevent* bev, void* context) {
  struct evbuffer* input = bufferevent_get_input(bev);
  struct evbuffer* output = bufferevent_get_output(bev);

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

void event_callback(struct bufferevent* bev, short error, void* context) {
  // User error & BEV_EVENT_x to determine which event type occurred.
  // EOF indicates that the client closed their socket, ERROR indicates a
  // transport layer error, and TIMEOUT is only issued if this was the
  // callback for a delay timer.
  bufferevent_free(bev);
}

void accept_callback(evutil_socket_t listener, short event, void *arg) {
  struct event_base* base = arg;
  struct sockaddr_storage their_addr;
  socklen_t size = sizeof(their_addr);

  int fd = accept(listener, (struct sockaddr*)&their_addr, &size);

  if (fd < 0) {
    LOG("Got invalid fd: %d", fd);
  } else if (fd > FD_SETSIZE) {
    close(fd);
  } else {
    char host[NI_MAXHOST];
    char port[NI_MAXSERV];
    int rc = getnameinfo(
      (struct sockaddr*)&their_addr, size, host, sizeof(host), port, sizeof(port),
      NI_NUMERICHOST | NI_NUMERICSERV);
    if (rc == 0) {
      LOG("Accepted connection from %s:%s", host, port);
    } else {
      LOG("Accepted connection from unknown host!");
    }

    struct bufferevent* bev;
    evutil_make_socket_nonblocking(fd);
    bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, read_callback, NULL, event_callback, NULL);
    bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
    bufferevent_enable(bev, EV_READ|EV_WRITE);
  }
}

bool run(void) {
  struct event_base* base = event_base_new();
  if (!base) {
    LOG("Failed to initialize base!");
    return false;
  }

  struct sockaddr_in sin;
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = 0;
  sin.sin_port = htons(1618);

  evutil_socket_t listener = socket(AF_INET, SOCK_STREAM, 0);
  evutil_make_socket_nonblocking(listener);

#ifndef WIN32
  {
    int one = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
  }
#endif

  if (bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
    LOG("Failed to bind socket!");
    return false;
  }

  if (listen(listener, 16) < 0) {
    LOG("Failed to listen on socket!");
    return false;
  }

  struct event* listener_event = event_new(
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
