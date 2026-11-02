#pragma once
#include <stdint.h>
typedef struct {
    uint32_t audio_pct, peak_pct, worker_us, wait_us, dropouts, faults, seconds, jobs;
    uint32_t dual, status, cpu0_us, cpu0_peak_us, worker_peak_us, wait_peak_us;
    uint32_t last_us, max_us, deadline_misses, blocks, cpu_mhz, scan_div;
} perf_stats_t;
void perf_read(perf_stats_t *s);
void perf_reset(void);
void perf_mode(int dual);
void perf_scan_mode(unsigned div);
