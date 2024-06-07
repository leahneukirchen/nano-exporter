#define _GNU_SOURCE
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

static void rapl_collect(scrape_req *req, void *ctx);

const struct collector rapl_collector = {
  .name = "rapl",
  .collect = rapl_collect,
};

static void rapl_collect_zone(scrape_req *req, char *path) {
  char name[BUF_SIZE];
  char buf[BUF_SIZE];

  if (read_file_at(AT_FDCWD, path, buf, sizeof buf) < 0)
    return;

  char *slash = strrchr(path, '/');
  if (!slash)
    return;
  *slash = 0;

  strcat(path, "/name");

  if (read_file_at(AT_FDCWD, path, name, sizeof name) < 0)
    return;

  *slash = 0;

  struct label rapl_label[] = {
    { .key = "rapl_zone", .value = path + strlen("/sys/class/powercap/") },
    { .key = "name", .value = name },
    LABEL_END,
  };

  scrape_write(req, "node_rapl_joules_total", rapl_label, atof(buf) / 1e6);
}

static void rapl_collect(scrape_req *req, void *ctx) {
  (void) ctx;

  // scan /sys/class/powercap

  glob_t globbuf = { 0 };
  if (glob("/sys/class/powercap/*/energy_uj", GLOB_NOSORT, 0, &globbuf) != 0)
    return;

  for (size_t i = 0; i < globbuf.gl_pathc; i++)
    rapl_collect_zone(req, globbuf.gl_pathv[i]);

  globfree(&globbuf);
}
