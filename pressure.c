#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "scrape.h"
#include "util.h"

// size of input buffer for paths and lines
#define BUF_SIZE 256

static void pressure_collect(scrape_req *req, void *ctx);

const struct collector pressure_collector = {
  .name = "pressure",
  .collect = pressure_collect,
};

static void pressure_collect(scrape_req *req, void *ctx) {
  (void) ctx;

  FILE *f;
  long long ms;

  if ((f = fopen("/proc/pressure/cpu", "r"))) {
    if (fscanf(f, "some %*[^t]total=%lld\n", &ms) == 1)
      scrape_write(req, "node_cpu_waiting_seconds_total", 0, ms / 1e6);
    fclose(f);
  }

  if ((f = fopen("/proc/pressure/memory", "r"))) {
    if (fscanf(f, "some %*[^t]total=%lld\n", &ms) == 1)
      scrape_write(req, "memory_waiting_seconds_total", 0, ms / 1e6);
    if (fscanf(f, "full %*[^t]total=%lld\n", &ms) == 1)
      scrape_write(req, "memory_stalled_seconds_total", 0, ms / 1e6);
    fclose(f);
  }

  if ((f = fopen("/proc/pressure/io", "r"))) {
    if (fscanf(f, "some %*[^t]total=%lld\n", &ms) == 1)
      scrape_write(req, "io_waiting_seconds_total", 0, ms / 1e6);
    if (fscanf(f, "full %*[^t]total=%lld\n", &ms) == 1)
      scrape_write(req, "io_stalled_seconds_total", 0, ms / 1e6);
    fclose(f);
  }
}
