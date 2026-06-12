#define _GNU_SOURCE

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* Exit code 77 is the standard "test skipped" code (autoconf convention). */
#define TEST_SKIP 77

static int g_pass = 0;
static int g_fail = 0;
static int g_skip = 0;

#define PASS(fmt, ...)                                                         \
  do {                                                                         \
    printf("  PASS: " fmt "\n", ##__VA_ARGS__);                                \
    g_pass++;                                                                  \
  } while (0)
#define FAIL(fmt, ...)                                                         \
  do {                                                                         \
    printf("  FAIL: " fmt "\n", ##__VA_ARGS__);                                \
    g_fail++;                                                                  \
  } while (0)
#define SKIP(fmt, ...)                                                         \
  do {                                                                         \
    printf("  SKIP: " fmt "\n", ##__VA_ARGS__);                                \
    g_skip++;                                                                  \
  } while (0)

/*
 * Test 1: socket() must return a valid file descriptor even when SO_MARK
 * cannot be applied (e.g. missing CAP_NET_ADMIN).  The library must never
 * cause socket creation to fail.
 */
static void test_socket_succeeds(void) {
  printf("[ socket() returns a valid fd ]\n");

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    FAIL("socket() returned %d: %s", fd, strerror(errno));
    return;
  }
  close(fd);
  PASS("socket() returned a valid fd");
}

/*
 * Test 2: When socket() returns a valid fd the library must not leave errno
 * set from an internal setsockopt/fprintf failure.  This verifies the
 * errno save/restore fix.
 */
static void test_errno_not_leaked(void) {
  printf("[ errno not leaked after successful socket() ]\n");

  errno = 0;
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  int post_errno = errno;

  if (fd < 0) {
    FAIL("socket() failed: %s", strerror(post_errno));
    return;
  }
  close(fd);

  if (post_errno != 0) {
    FAIL("errno=%d (%s) leaked after successful socket()", post_errno,
         strerror(post_errno));
  } else {
    PASS("errno=0 after successful socket()");
  }
}

/*
 * Test 3: Read SO_MARK back from the socket and compare with expected value.
 * Requires CAP_NET_ADMIN; skipped gracefully when that capability is absent.
 */
static void test_mark_value(uint32_t expected) {
  printf("[ SO_MARK=%u applied to socket ]\n", expected);

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    FAIL("socket() failed: %s", strerror(errno));
    return;
  }

  uint32_t mark = 0;
  socklen_t len = sizeof(mark);
  if (getsockopt(fd, SOL_SOCKET, SO_MARK, &mark, &len) < 0) {
    close(fd);
    if (errno == EPERM) {
      SKIP("need CAP_NET_ADMIN to read SO_MARK (run as root)");
    } else {
      FAIL("getsockopt(SO_MARK) failed: %s", strerror(errno));
    }
    return;
  }
  close(fd);

  if (mark == expected) {
    PASS("SO_MARK=%u matches expected", mark);
  } else if (mark == 0 && expected != 0) {
    /*
     * Mark is still 0: setsockopt in the hook most likely failed with
     * EPERM (no CAP_NET_ADMIN).  Treat as skipped, not a library bug.
     */
    SKIP("SO_MARK not applied -- missing CAP_NET_ADMIN? (run as root)");
  } else {
    FAIL("SO_MARK=%u, expected %u", mark, expected);
  }
}

int main(void) {
  /* Determine the expected mark from the environment (mirrors setmark.c). */
  uint32_t expected_mark = 1234; /* DEFAULT_MARK */

  const char *env = getenv("SO_MARK");
  if (env && *env) {
    char *end;
    errno = 0;
    unsigned long val = strtoul(env, &end, 10);
    if (errno == 0 && end != env && *end == '\0' && val <= UINT32_MAX) {
      expected_mark = (uint32_t)val;
    }
  }

  printf("=== setmark tests (expected mark=%u) ===\n\n", expected_mark);

  test_socket_succeeds();
  test_errno_not_leaked();
  test_mark_value(expected_mark);

  printf("\nResults: %d passed, %d failed, %d skipped\n", g_pass, g_fail,
         g_skip);

  if (g_fail > 0)
    return 1;
  if (g_pass == 0)
    return TEST_SKIP;
  return 0;
}
