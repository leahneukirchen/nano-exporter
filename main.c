/*
 * Copyright 2018 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _POSIX_C_SOURCE 200809L

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "scrape.h"
#include "util.h"

#ifndef XCOLLECTORS
#define XCOLLECTORS \
  X(conntrack) \
  X(cpu) \
  X(diskstats) \
  X(filefd) \
  X(filesystem) \
  X(hwmon) \
  X(loadavg) \
  X(mdstat) \
  X(meminfo) \
  X(netdev) \
  X(pressure) \
  X(rapl) \
  X(stat) \
  X(textfile) \
  X(timex) \
  X(uname) \
  X(vmstat) \

#endif

#define X(c) extern const struct collector c##_collector;
XCOLLECTORS
#undef X

#define X(c) &c##_collector,
static const struct collector *collectors[] = {
  XCOLLECTORS
};
#undef X
#define NCOLLECTORS ((unsigned) (sizeof collectors / sizeof *collectors))


struct config {
  const char *port;
  bool print;
};

const struct config default_config = {
  .port = "9100",
  .print = false,
};

struct collector_ctx {
  unsigned enabled;
  const struct collector *coll[NCOLLECTORS];
  void *coll_ctx[NCOLLECTORS];
};

static bool initialize(int argc, char *argv[], struct config *cfg, struct collector_ctx *ctx);

int main(int argc, char *argv[]) {
  struct config cfg = default_config;
  struct collector_ctx ctx;

  if (!initialize(argc, argv, &cfg, &ctx))
    return 1;

  if (cfg.print) {
    scrape_print(ctx.enabled, ctx.coll, ctx.coll_ctx);
    return 0;
  }

  scrape_server *server = scrape_listen(cfg.port);
  if (!server)
    return 1;

  scrape_serve(server, ctx.enabled, ctx.coll, ctx.coll_ctx);
  scrape_close(server);

  return 0;
}

static bool initialize(int argc, char *argv[], struct config *cfg, struct collector_ctx *ctx) {
  // parse command-line arguments

  enum tristate { flag_off = -1, flag_undef = 0, flag_on = 1 };
  enum tristate coll_enabled[NCOLLECTORS];
  struct { struct slist *args; struct slist **next_arg; unsigned count; } coll_args[NCOLLECTORS];

  for (unsigned i = 0; i < NCOLLECTORS; i++) {
    coll_enabled[i] = flag_undef;
    coll_args[i].args = 0;
    coll_args[i].next_arg = &coll_args[i].args;
    coll_args[i].count = 0;
  }

  enum tristate enabled_default = flag_on;

  for (int arg = 1; arg < argc; arg++) {
    // check for collector arguments

    for (unsigned i = 0; i < NCOLLECTORS; i++) {
      size_t name_len = strlen(collectors[i]->name);
      if (strncmp(argv[arg], "--", 2) == 0
          && strncmp(argv[arg] + 2, collectors[i]->name, name_len) == 0
          && argv[arg][2 + name_len] == '-') {
        char *carg = argv[arg] + 2 + name_len + 1;
        if (strcmp(carg, "on") == 0) {
          coll_enabled[i] = flag_on;
          enabled_default = flag_off;
        } else if (strcmp(carg, "off") == 0) {
          coll_enabled[i] = flag_off;
        } else if (collectors[i]->init && collectors[i]->has_args) {
          coll_args[i].next_arg = slist_append(coll_args[i].next_arg, carg);
          coll_args[i].count++;
        } else {
          fprintf(stderr, "unknown argument: %s (collector %s takes no arguments)\n", argv[arg], collectors[i]->name);
          return false;
        }
        goto next_arg;
      }
    }

    // parse any non-collector arguments

    // TODO --help
    if (strncmp(argv[arg], "--port=", 7) == 0) {
      cfg->port = &argv[arg][7];
      goto next_arg;
    }

    if (strncmp(argv[arg], "--stdout", 8) == 0) {
      cfg->print = true;
      goto next_arg;
    }

    fprintf(stderr, "unknown argument: %s\n", argv[arg]);
    return false;
 next_arg: ;
  }

  unsigned max_args = 1;

  for (unsigned i = 0; i < NCOLLECTORS; i++) {
    if (coll_enabled[i] == flag_undef)
      coll_enabled[i] = enabled_default;
    if (coll_enabled[i] == flag_on && coll_args[i].count > max_args)
      max_args = coll_args[i].count;
  }

  // prepare the list of enabled collectors, and initialize them

  unsigned coll_argc;
  char *coll_argv[max_args + 1];

  ctx->enabled = 0;

  for (unsigned i = 0; i < NCOLLECTORS; i++) {
    if (coll_enabled[i] != flag_on)
      continue;

    unsigned c = ctx->enabled++;
    ctx->coll[c] = collectors[i];

    if (collectors[i]->init) {
      coll_argc = 0;
      for (struct slist *arg = coll_args[i].args; arg && coll_argc < max_args; arg = arg->next)
        coll_argv[coll_argc++] = arg->data;
      coll_argv[coll_argc] = 0;

      ctx->coll_ctx[c] = collectors[i]->init(coll_argc, coll_argv);

      if (!ctx->coll_ctx[c]) {
        fprintf(stderr, "failed to initialize collector %s\n", collectors[i]->name);
        return false;
      }
    } else {
      ctx->coll_ctx[c] = 0;
    }
  }

  return true;
}
