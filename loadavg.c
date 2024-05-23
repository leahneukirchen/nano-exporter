#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "scrape.h"
#include "util.h"

// size of input buffer for paths and lines
#define BUF_SIZE 256

static void loadavg_collect(scrape_req *req, void *ctx);

const struct collector loadavg_collector = {
  .name = "loadavg",
  .collect = loadavg_collect,
};

static void loadavg_collect(scrape_req *req, void *ctx) {
  (void) ctx;

  double loadavg[3];
  if (getloadavg(loadavg, 3) == 3) {
    scrape_write(req, "node_load1", 0, loadavg[0]);
    scrape_write(req, "node_load5", 0, loadavg[1]);
    scrape_write(req, "node_load15", 0, loadavg[2]);
  }

  struct timespec ts;
  if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) < 0)
    return;
  double d = ts.tv_sec + ts.tv_nsec / 1e9;
  scrape_write(req, "process_cpu_seconds_total", 0, d);
}
