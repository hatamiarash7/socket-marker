#define _GNU_SOURCE

#include <arpa/inet.h>
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

static const uint32_t DEFAULT_MARK = 1234;

static uint32_t socket_mark = DEFAULT_MARK;
static int (*real_socket)(int, int, int) = NULL;

__attribute__((constructor)) static void init_socket_marker(void) {
  const char *env = getenv("SO_MARK");

  if (env && *env) {
    char *end = NULL;
    errno = 0;

    unsigned long value = strtoul(env, &end, 10);

    if (errno == 0 && end != env && *end == '\0' && value <= UINT32_MAX) {
      socket_mark = (uint32_t)value;
    } else {
      fprintf(stderr,
              "[Socket Marker] Invalid SO_MARK='%s', using default %u\n", env,
              DEFAULT_MARK);
    }
  }

  dlerror(); /* Clear any old error */

  real_socket = dlsym(RTLD_NEXT, "socket");

  const char *err = dlerror();
  if (err != NULL) {
    fprintf(stderr, "[Socket Marker] dlsym(socket) failed: %s\n", err);
  } else {
    fprintf(stderr, "[Socket Marker] Initialized (mark=%u)\n", socket_mark);
  }
}

int socket(int domain, int type, int protocol) {
  if (!real_socket) {
    errno = ENOSYS;
    return -1;
  }

  int fd = real_socket(domain, type, protocol);

  if (fd < 0)
    return fd;

  if (setsockopt(fd, SOL_SOCKET, SO_MARK, &socket_mark, sizeof(socket_mark)) <
      0) {
    fprintf(stderr, "[Socket Marker] setsockopt(fd=%d, mark=%u) failed: %m\n",
            fd, socket_mark);
  }

  return fd;
}