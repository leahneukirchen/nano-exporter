#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scrape.h"
#include "util.h"

static void filefd_collect(scrape_req *req, void *ctx);

const struct collector filefd_collector = {
  .name = "filefd",
  .collect = filefd_collect,
};

static void filefd_collect(scrape_req *req, void *ctx) {
  (void) ctx;

  FILE *f;

  // scan /proc/sys/fs/file-nr for metrics

  f = fopen(PATH("/proc/sys/fs/file-nr"), "r");
  if (!f)
    return;

  double alloc, ignored, max;
  if (fscanf(f, "%lf %lf %lf", &alloc, &ignored, &max) == 3) {
    scrape_write(req, "node_filefd_allocated", 0, alloc);
    scrape_write(req, "node_filefd_maximum", 0, max);
  }

  fclose(f);
}
