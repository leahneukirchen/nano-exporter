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

static void netif_collect(scrape_req *req, void *ctx);

const struct collector netif_collector = {
  .name = "netif",
  .collect = netif_collect,
};

static void netif_collect_dir(scrape_req *req, char *dir) {
  char buf[BUF_SIZE];
  char dev[32] = { 0 };
  strcpy(dev, dir + 15 /* "/sys/class/net" */);

  int dirfd = open(dir, O_PATH);
  if (dirfd < 0)
    return;

  struct label dev_label[] = {
    { .key = "device", .value = dev },
    LABEL_END,
  };

  if (read_file_at(dirfd, "mtu", buf, sizeof buf) > 0)
    scrape_write(req, "node_network_mtu_bytes", dev_label, atof(buf));

  if (read_file_at(dirfd, "carrier", buf, sizeof buf) > 0) {
    scrape_write(req, "node_network_carrier", dev_label, atof(buf));

    if (read_file_at(dirfd, "carrier_changes", buf, sizeof buf) > 0)
      scrape_write(req, "node_network_carrier_changes_total", dev_label, atof(buf));
  }

  if (read_file_at(dirfd, "operstate", buf, sizeof buf) > 0) {
    scrape_write(req, "node_network_up", dev_label, (float)(strcmp(buf, "down") == 0));

    if (strcmp(buf, "up") == 0 &&
        read_file_at(dirfd, "speed", buf, sizeof buf) > 0 &&
        atof(buf) > 0)
      scrape_write(req, "node_network_speed_bytes", dev_label,
          atof(buf) * 125000.0); /* Mbit -> bytes */
  }

  close(dirfd);
}

static void netif_collect(scrape_req *req, void *ctx) {
  (void) ctx;

  // scan /sys/class/net for metrics

  glob_t globbuf = { 0 };
  if (glob("/sys/class/net/*", GLOB_NOSORT | GLOB_ONLYDIR, 0, &globbuf) != 0)
    return;

  for (size_t i = 0; i < globbuf.gl_pathc; i++)
    netif_collect_dir(req, globbuf.gl_pathv[i]);

  globfree(&globbuf);
}
