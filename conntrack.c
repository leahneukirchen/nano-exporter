#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "scrape.h"
#include "util.h"

// size of input buffer for paths and lines
#define BUF_SIZE 256

static void conntrack_collect(scrape_req *req, void *ctx);

const struct collector conntrack_collector = {
  .name = "conntrack",
  .collect = conntrack_collect,
};

static void conntrack_collect(scrape_req *req, void *ctx) {
  (void) ctx;

  FILE *f;
  double d;

  f = fopen(PATH("/proc/sys/net/netfilter/nf_conntrack_count"), "r");
  if (!f)
    return;

  if (fscanf(f, "%lf", &d) == 1)
    scrape_write(req, "node_nf_conntrack_entries", 0, d);

  fclose (f);

  f = fopen(PATH("/proc/sys/net/netfilter/nf_conntrack_max"), "r");
  if (!f)
    return;

  if (fscanf(f, "%lf", &d) == 1)
    scrape_write(req, "node_nf_conntrack_entries_limit", 0, d);

  fclose (f);
}
