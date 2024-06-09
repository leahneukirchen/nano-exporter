#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "scrape.h"
#include "util.h"

// size of input buffer for paths and lines
#define BUF_SIZE 256

static void schedstat_collect(scrape_req *req, void *ctx);

const struct collector schedstat_collector = {
  .name = "schedstat",
  .collect = schedstat_collect,
};

static void schedstat_collect(scrape_req *req, void *ctx) {
  (void) ctx;

  FILE *f = fopen("/proc/schedstat", "r");
  if (!f)
    return;

  char cpu[8];
  long long running, waiting, timeslices;

  char line[BUF_SIZE];
  while(fgets_line(line, sizeof line, f)) {
    if (sscanf(line, "cpu%[0-9] %*d %*d %*d %*d %*d %*d %lld %lld %lld\n",
        cpu, &running, &waiting, &timeslices) == 4) {
      struct label labels[] = {
        { .key = "cpu", .value = cpu },
        LABEL_END,
      };
      scrape_write(req, "node_schedstat_running_seconds_total", labels, running / 1e9);
      scrape_write(req, "node_schedstat_waiting_seconds_total", labels, waiting / 1e9);
      scrape_write(req, "node_schedstat_timeslices_total", labels, (double)timeslices);
    }
  }

  fclose(f);
}
