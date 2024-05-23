#include <sys/timex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scrape.h"
#include "util.h"

#define NTP_PPS 0

// size of input buffer for paths and lines
#define BUF_SIZE 256

static void timex_collect(scrape_req *req, void *ctx);

const struct collector timex_collector = {
  .name = "timex",
  .collect = timex_collect,
};

static void timex_collect(scrape_req *req, void *ctx) {
  (void) ctx;

  struct timex buf = { 0 };

  int r = adjtimex(&buf);
  if (r < 0)
    return;

  scrape_write(req, "node_timex_sync_status", 0, r != TIME_ERROR);

  scrape_write(req, "node_timex_estimated_error_seconds", 0, buf.esterror / 1e6);
  scrape_write(req, "node_timex_frequency_adjustment_ratio", 0, 1 + buf.freq/(1000000.0 * 65536.0));
  scrape_write(req, "node_timex_loop_time_constant", 0, (double)buf.constant);
  scrape_write(req, "node_timex_maxerror_seconds", 0, buf.maxerror / 1e6);
  scrape_write(req, "node_timex_offset_seconds", 0, buf.offset / 1e6);
  scrape_write(req, "node_timex_status", 0, (double)buf.status);
  scrape_write(req, "node_timex_tai_offset_seconds", 0, (double)buf.tai);
  scrape_write(req, "node_timex_tick_seconds", 0, buf.tick / 1e6);

#if NTP_PPS
  scrape_write(req, "node_timex_pps_calibraton_total", 0, (double)buf.calcnt);
  scrape_write(req, "node_timex_pps_error_total", 0, (double)buf.errcnt);
  scrape_write(req, "node_timex_pps_frequency_hertz", 0, buf.ppsfreq / (1000000.0 * 65536.0));
  scrape_write(req, "node_timex_pps_jitter_seconds", 0, buf.jitter / 1e6);
  scrape_write(req, "node_timex_pps_jitter_total", 0, (double)buf.jitcnt);
  scrape_write(req, "node_timex_pps_shift_seconds", 0, (double)buf.shift);
  scrape_write(req, "node_timex_pps_stability_exceeded_total", 0, (double)buf.stbcnt);
  scrape_write(req, "node_timex_pps_stability_hertz", 0, buf.stabil / (1000000.0 * 65536.0));
#endif
}
