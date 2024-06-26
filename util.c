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

#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "util.h"

// character buffers

struct bbuf {
  char *data;
  size_t len;
  size_t size;
  size_t max_size;
};

static bool bbuf_reserve(bbuf *buf, size_t len) {
  if (buf->len + len <= buf->size)
    return true;

  size_t new_size = buf->size;
  while (buf->len + len > new_size && new_size < buf->max_size)
    new_size *= 2;
  if (buf->len + len > new_size)
    return false;

  char *new_data = must_realloc(buf->data, new_size);
  buf->data = new_data;
  buf->size = new_size;
  return true;
}

bbuf *bbuf_alloc(size_t initial_size, size_t max_size) {
  bbuf *buf = must_malloc(sizeof *buf);
  buf->data = must_malloc(initial_size);
  buf->len = 0;
  buf->size = initial_size;
  buf->max_size = max_size;
  return buf;
}

void bbuf_free(bbuf *buf) {
  free(buf->data);
  free(buf);
}

void bbuf_reset(bbuf *buf) {
  buf->len = 0;
}

size_t bbuf_len(bbuf *buf) {
  return buf->len;
}

void bbuf_put(bbuf *buf, const void *src, size_t len) {
  if (!bbuf_reserve(buf, len))
    return;
  memcpy(buf->data + buf->len, src, len);
  buf->len += len;
}

void bbuf_puts(bbuf *buf, const char *src) {
  bbuf_put(buf, src, strlen(src));
}

void bbuf_putc(bbuf *buf, int c) {
  if (!bbuf_reserve(buf, 1))
    return;
  buf->data[buf->len++] = c;
}

void bbuf_putf(bbuf *buf, const char *fmt, ...) {
  va_list ap;

  int len = 0;
  va_start(ap, fmt);
  len = vsnprintf(0, 0, fmt, ap);
  va_end(ap);

  if (len > 0) {
    if (!bbuf_reserve(buf, len + 1))
      return;
    va_start(ap, fmt);
    vsnprintf(buf->data + buf->len, len + 1, fmt, ap);
    va_end(ap);
    buf->len += len;
  }
}

char *bbuf_get(struct bbuf *buf, size_t *len) {
  *len = buf->len;
  return buf->data;
}

int bbuf_cmp(bbuf *buf, const char *other) {
  size_t other_len = strlen(other);
  if (buf->len < other_len)
    return -1;
  else if (buf->len == other_len)
    return memcmp(buf->data, other, buf->len);
  else
    return +1;
}

// string lists

struct slist *slist_split(const char *str, const char *delim) {
  struct slist *list = 0, **prev = &list, *next;

  while (*str) {
    size_t span = strcspn(str, delim);
    if (span == 0) {
      str++;
      continue;
    }

    next = must_malloc(sizeof *next + span + 1);
    memcpy(next->data, str, span);
    next->data[span] = '\0';
    *prev = next;
    prev = &next->next;

    str += span;
    while (*str && strchr(delim, *str))
      str++;
  }
  *prev = 0;

  return list;
}

struct slist **slist_append(struct slist **prev, const char *str) {
  size_t len = strlen(str);
  struct slist *new = *prev = must_malloc(sizeof *new + len + 1);
  new->next = 0;
  memcpy(new->data, str, len + 1);
  return &new->next;
}

struct slist *slist_prepend(struct slist *list, const char *str) {
  size_t len = strlen(str);
  struct slist *new = must_malloc(sizeof *new + len + 1);
  new->next = list;
  memcpy(new->data, str, len + 1);
  return new;
}

bool slist_contains(const struct slist *list, const char *key) {
  for (; list; list = list->next)
    if (strcmp(list->data, key) == 0)
      return true;
  return false;
}

bool slist_matches(const struct slist *list, const char *key) {
  size_t key_len = strlen(key);

  for (; list; list = list->next) {
    size_t match_len = strlen(list->data);
    if (match_len > 0 && list->data[match_len-1] == '*') {
      if (match_len-1 <= key_len && memcmp(list->data, key, match_len-1) == 0)
        return true;
    } else {
      if (match_len == key_len && memcmp(list->data, key, key_len) == 0)
        return true;
    }
  }
  return false;
}

// miscellaneous utilities

void *must_malloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr) {
    perror("malloc");
    abort();
  }
  return ptr;
}

void *must_realloc(void *ptr, size_t size) {
  void *new_ptr = realloc(ptr, size);
  if (!new_ptr) {
    perror("realloc");
    abort();
  }
  return new_ptr;
}

char *must_strdup(const char *src) {
  char *dst = strdup(src);
  if (!dst) {
    perror("strdup");
    abort();
  }
  return dst;
}

char *fgets_line(char *s, int size, FILE *stream) {
  s[size - 1] = '\0';
  char *got = fgets(s, size, stream);
  if (!got)
    return 0;
  if (s[size - 1] == '\0' || s[size - 1] == '\n')
    return got;

  int c;
  while ((c = fgetc(stream)) != EOF)
    if (c == '\n')
      return got;
  return 0;
}

int write_all(int fd, const void *buf_ptr, size_t len) {
  const char *buf = buf_ptr;

  while (len > 0) {
    ssize_t wrote = write(fd, buf, len);
    if (wrote <= 0)
      return -1;
    buf += wrote;
    len -= wrote;
  }

  return 0;
}

ssize_t
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
