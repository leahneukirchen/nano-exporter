#define _GNU_SOURCE
#include <ctype.h>
#include <fcntl.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "scrape.h"
#include "util.h"

// size of input buffer for paths and lines
#define BUF_SIZE 256

static void zfs_collect(scrape_req *req, void *ctx);

const struct collector zfs_collector = {
  .name = "zfs",
  .collect = zfs_collect,
};

static void zfs_collect_state(scrape_req *req, char *path) {
  char buf[BUF_SIZE];
  char pool[BUF_SIZE] = { 0 };
  memcpy(pool, path + 20 /* "/proc/spl/kstat/zfs/" */, strlen(path) - 20 - 6 /* "/state" */);

  if (read_file_at(AT_FDCWD, path, buf, sizeof buf) < 0)
    return;

  for (char *s = buf; *s; s++)
    *s = tolower(*s);

  struct label zfs_label[] = {
    { .key = "device", .value = pool },
    { .key = "state", .value = buf },
    LABEL_END,
  };

  scrape_write(req, "node_zfs_zpool_state", zfs_label, 1.0);
}

static void zfs_collect(scrape_req *req, void *ctx) {
  (void) ctx;

  // scan /proc/spl/kstat/zfs/*/state for metrics

  glob_t globbuf = { 0 };
  if (glob("/proc/spl/kstat/zfs/*/state", GLOB_NOSORT | GLOB_ONLYDIR, 0, &globbuf) != 0)
    return;

  for (size_t i = 0; i < globbuf.gl_pathc; i++)
    zfs_collect_state(req, globbuf.gl_pathv[i]);

  globfree(&globbuf);
}
