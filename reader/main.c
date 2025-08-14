#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#ifdef TIMER32BIT
#define RECORD_SIZE 8
#else
#define RECORD_SIZE 6
#endif
#define BUFFER_SIZE (300 * RECORD_SIZE)

int configure_serial(int fd, speed_t speed) {
  struct termios tty;
  if (tcgetattr(fd, &tty) != 0) {
    perror("tcgetattr");
    return -1;
  }

  cfmakeraw(&tty); // Disable input/output processing

  // Set baud rate (ignored by USB CDC but required for termios)
  if (cfsetispeed(&tty, speed) != 0 || cfsetospeed(&tty, speed) != 0) {
    perror("cfset[io]speed");
    return -1;
  }

  tty.c_cc[VMIN] = RECORD_SIZE; // Block until full record
  tty.c_cc[VTIME] = 0;

  if (tcsetattr(fd, TCSANOW, &tty) != 0) {
    perror("tcsetattr");
    return -1;
  }

  return 0;
}

void usage(const char *prog) {
  fprintf(stderr, "Usage: %s /dev/ttyXXX\n", prog);
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    usage(argv[0]);
  }

  const char *device = argv[1];
  uint8_t idx = 0;

  int fd = open(device, O_RDONLY | O_NOCTTY | O_NONBLOCK);
  if (fd < 0) {
    perror("open");
    return 1;
  }

  if (configure_serial(fd, B9600) != 0) {
    close(fd);
    return 1;
  }

  uint8_t buf[BUFFER_SIZE];
  uint8_t *b = buf;
  for (uint8_t cnt = 0;;) {
    usleep(10000);
    ssize_t n = read(fd, b, BUFFER_SIZE - (b - buf));
    if (n == -1 && errno == EAGAIN) {
      continue;
    } else if (n == -1) {
      perror("read");
      break;
    }

    b += n;

  next:
    // synchronize with stream
    while (b > buf && buf[0] != 0xAA) {
      b--;
      memmove(buf, buf + 1, BUFFER_SIZE - 1);
    }
    if (b == buf)
      goto restart;

    if (b - buf < RECORD_SIZE)
      continue;

    if (buf[RECORD_SIZE - 1] != 0xBB)
      goto consume;

    idx = 1;
    uint8_t counter = 0;
    counter |= ((uint8_t)buf[idx++] << 0);

    uint32_t timestamp = 0;
#ifdef TIMER32BIT
    timestamp |= ((uint32_t)buf[idx++] << 24);
    timestamp |= ((uint32_t)buf[idx++] << 16);
#endif
    timestamp |= ((uint32_t)buf[idx++] << 8);
    timestamp |= ((uint32_t)buf[idx++] << 0);

    uint8_t pinb = buf[idx++];

    if (counter != ++cnt) {
      cnt = counter;
    }
    assert(++idx == RECORD_SIZE);

    printf("%u %u %u\n", counter, timestamp, pinb);
    fflush(stdout);

  consume:
    memmove(buf, buf + RECORD_SIZE, BUFFER_SIZE - RECORD_SIZE);
    b -= RECORD_SIZE;
    assert(b >= buf);
    goto next;

  restart:
    b = buf;
  }

  close(fd);
  return 0;
}
