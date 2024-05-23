#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scrape.h"
#include "util.h"

// size of input buffer for paths and lines
#define BUF_SIZE 256

static void vmstat_collect(scrape_req *req, void *ctx);

const struct collector vmstat_collector = {
  .name = "vmstat",
  .collect = vmstat_collect,
};

static const struct {
  const char *name;
  const char *key;
  unsigned key_len;
} metrics[] = {
  { .name = "node_vmstat_oom_kill", .key = "oom_kill ", .key_len = 9 },
  { .name = "node_vmstat_pgfault",  .key = "pgfault ", .key_len = 8 },
  { .name = "node_vmstat_pgmajfault",  .key = "pgmajfault ", .key_len = 11 },
  { .name = "node_vmstat_pgpgin", .key = "pgpgin ", .key_len = 7 },
  { .name = "node_vmstat_pgpgout", .key = "pgpgout ", .key_len = 8 },
  { .name = "node_vmstat_pswpin", .key = "pswpin ", .key_len = 7 },
  { .name = "node_vmstat_pswpout", .key = "pswpout ", .key_len = 8 },
};
#define NMETRICS (sizeof metrics / sizeof *metrics)

static void vmstat_collect(scrape_req *req, void *ctx) {
  (void) ctx;

  // buffers

  char buf[BUF_SIZE];

  FILE *f;

  // scan /proc/stat for metrics

  f = fopen(PATH("/proc/vmstat"), "r");
  if (!f)
    return;

  while (fgets_line(buf, sizeof buf, f)) {
    for (size_t m = 0; m < NMETRICS; m++) {
      if (strncmp(buf, metrics[m].key, metrics[m].key_len) != 0)
        continue;

      char *end;
      double d = strtod(buf + metrics[m].key_len, &end);
      if (*end != '\0' && *end != ' ' && *end != '\n')
        continue;

      scrape_write(req, metrics[m].name, 0, d);
      break;
    }
  }

  fclose(f);
}
