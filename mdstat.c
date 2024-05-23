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

static void mdstat_collect(scrape_req *req, void *ctx);

const struct collector mdstat_collector = {
  .name = "mdstat",
  .collect = mdstat_collect,
};

static ssize_t
read_file_at(int dirfd, char *pathname, char *buf, size_t bufsiz) {
  int fd = openat(dirfd, pathname, O_RDONLY);
  if (fd < 0)
    return -1;

  ssize_t r = read(fd, buf, bufsiz - 1);
  close(fd);
  if (r < 0)
    return -1;

  if (buf[r-1] == '\n')
    r--;

  buf[r] = 0;
  return r;
}

void
scrape_write_md_label(scrape_req *req, char *name, char *md, char *label, char *value, double d) {
  struct label labels[] = {
    { .key = "device", .value = md },
    { .key = label, .value = value },
    LABEL_END,
  };
  scrape_write(req, name, labels, d);
}

static void mdstat_collect_dir(scrape_req *req, char *dir) {
  char buf[BUF_SIZE];
  char md[32] = { 0 };
  memcpy(md, dir + 11 /* "/sys/block/" */, strlen(dir) - 11 - 3 /* "/md" */);

  int dirfd = open(dir, O_PATH);
  if (dirfd < 0)
    return;

  struct label md_label[] = {
    { .key = "device", .value = md },
    LABEL_END,
  };

  if (read_file_at(dirfd, "level", buf, sizeof buf) > 0)
    scrape_write_md_label(req, "node_md_level", md, "level", buf, 1);

  if (read_file_at(dirfd, "raid_disks", buf, sizeof buf) > 0)
    scrape_write(req, "node_md_disks", md_label, atof(buf));

  if (read_file_at(dirfd, "metadata_version", buf, sizeof buf) > 0)
    scrape_write_md_label(req, "node_md_metadata_version", md, "version", buf, 1);

  if (read_file_at(dirfd, "array_state", buf, sizeof buf) > 0)
    scrape_write_md_label(req, "node_md_state", md, "state", buf, 1);

  // uuid

  if (read_file_at(dirfd, "chunk_size", buf, sizeof buf) > 0)
    scrape_write(req, "node_md_chunk_size", md_label, atof(buf));

  if (read_file_at(dirfd, "degraded", buf, sizeof buf) > 0)
    scrape_write(req, "node_md_degraded_disks", md_label, atof(buf));

  if (read_file_at(dirfd, "sync_action", buf, sizeof buf) > 0)
    scrape_write_md_label(req, "node_md_sync_action", md, "action", buf, 1);

  if (read_file_at(dirfd, "sync_completed", buf, sizeof buf) > 0) {
    if (strcmp(buf, "none") != 0) {
      double a, b;
      if (sscanf(buf, "%lf / %lf", &a, &b) == 2)
        scrape_write(req, "node_md_sync_completed", md_label, a / b);
      if (read_file_at(dirfd, "sync_speed", buf, sizeof buf) > 0)
        scrape_write(req, "node_md_sync_speed", md_label, atof(buf));
    }
  }

  close(dirfd);

  char stateglob[BUF_SIZE];
  strcpy(stateglob, dir);
  strcat(stateglob, "/dev-*/state");

  glob_t globbuf = { 0 };
  if (glob(stateglob, GLOB_NOSORT, 0, &globbuf) != 0)
    return;

  for (size_t i = 0; i < globbuf.gl_pathc; i++) {
    char dev[16] = { 0 };
    memcpy(dev, globbuf.gl_pathv[i] + strlen(dir) + 5 /* "/dev-" */,
        strlen(globbuf.gl_pathv[i]) - strlen(dir) - 5 - 6 /* "/state" */);

    if (read_file_at(AT_FDCWD, globbuf.gl_pathv[i], buf, sizeof buf)) {
      struct label labels[] = {
        { .key = "device", .value = md },
        { .key = "disk", .value = dev },
        { .key = "state", .value = buf },
        LABEL_END,
      };
      scrape_write(req, "node_md_disk_state", labels, 1);
    }
  }

  globfree(&globbuf);
}

static void mdstat_collect(scrape_req *req, void *ctx) {
  (void) ctx;

  // scan /sys/block/md*/md/ for metrics

  if (access("/proc/mdstat", F_OK) != 0)
    return;

  glob_t globbuf = { 0 };
  if (glob("/sys/block/md*/md", GLOB_NOSORT | GLOB_ONLYDIR, 0, &globbuf) != 0)
    return;

  for (size_t i = 0; i < globbuf.gl_pathc; i++)
    mdstat_collect_dir(req, globbuf.gl_pathv[i]);

  globfree(&globbuf);
}
