#include <stdbool.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "textfile.h"
#include "util.h"

// TODO: check default
#define DEFAULT_DIR "/var/lib/prometheus-node-exporter"

void *textfile_init(int argc, char *argv[]);
void textfile_collect(scrape_req *req, void *ctx);

const struct collector textfile_collector = {
  .name = "textfile",
  .collect = textfile_collect,
  .init = textfile_init,
  .has_args = true,
};

void *textfile_init(int argc, char *argv[]) {
  char *dir = DEFAULT_DIR;

  for (int arg = 0; arg < argc; arg++) {
    if (strncmp(argv[arg], "dir=", 4) == 0) {
      dir = &argv[arg][4];
    } else {
      fprintf(stderr, "unknown argument for textfile collector: %s", argv[arg]);
      return 0;
    }
  }

  return dir;
}

void textfile_collect(scrape_req *req, void *ctx) {
  const char *dir = ctx;

  DIR *d = opendir(dir);
  if (!d)
    return;

  struct dirent *dent;
  char buf[4096];

  while ((dent = readdir(d))) {
    size_t name_len = strlen(dent->d_name);
    if (name_len < 6 || strcmp(dent->d_name + name_len - 5, ".prom") != 0)
      continue;

    snprintf(buf, sizeof buf, "%s/%s", dir, dent->d_name);
    FILE *f = fopen(buf, "r");
    if (!f)
      continue;

    bool has_newline = true;
    size_t len;
    while ((len = fread(buf, 1, sizeof buf, f)) > 0) {
      scrape_write_raw(req, buf, len);
      has_newline = buf[len - 1] == '\n';
    }
    if (!has_newline)
      scrape_write_raw(req, (char[]){'\n'}, 1);

    fclose(f);
  }

  closedir(d);
}