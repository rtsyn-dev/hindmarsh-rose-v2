#pragma once
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hindmarsh_rose_v2_state hindmarsh_rose_v2_state_t;

hindmarsh_rose_v2_state_t *hindmarsh_rose_v2_new(void);
void hindmarsh_rose_v2_free(hindmarsh_rose_v2_state_t *state);
void hindmarsh_rose_v2_set_config(hindmarsh_rose_v2_state_t *state,
                                    const char *key, size_t len, double value);
void hindmarsh_rose_v2_set_input(hindmarsh_rose_v2_state_t *state,
                                   const char *name, size_t len, double value);
void hindmarsh_rose_v2_process(hindmarsh_rose_v2_state_t *state);
double hindmarsh_rose_v2_get_output(const hindmarsh_rose_v2_state_t *state,
                                      const char *name, size_t len);
double hindmarsh_rose_v2_select_optimal_dt(double pts_match);
double hindmarsh_rose_v2_select_pts_burst(double burst_duration, double period_seconds);

#ifdef __cplusplus
}
#endif
