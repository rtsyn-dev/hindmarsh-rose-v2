#include "hindmarsh_rose_v2.h"
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

struct hindmarsh_rose_v2_state {
  double x;
  double y;
  double z;
  double input_syn;
  double e;
  double mu;
  double s;
  double vh;
  double dt;
  double burst_duration;
  double period_seconds;
  size_t s_points;
  double cfg_x;
  double cfg_y;
  double cfg_z;
};

static void hindmarsh_rose_v2_update_burst_settings(hindmarsh_rose_v2_state_t *state);
static int key_eq(const char *key, size_t len, const char *lit);
typedef void (*rk4_deriv_fn_t)(const double *state, double *deriv, size_t n,
                               void *user_data);
extern void rtsyn_plugin_rk4_step_n(double *state, size_t n, double dt,
                                    rk4_deriv_fn_t deriv_fn,
                                    void *user_data);

typedef struct hindmarsh_rose_v2_dynamics_ctx {
  double input_syn;
  double e;
  double mu;
  double s;
  double vh;
} hindmarsh_rose_v2_dynamics_ctx_t;

static void hindmarsh_rose_v2_deriv(const double *vars, double *deriv,
                                      size_t n, void *user_data) {
  (void)n;
  if (vars == NULL || deriv == NULL || user_data == NULL) {
    return;
  }
  const hindmarsh_rose_v2_dynamics_ctx_t *ctx =
      (const hindmarsh_rose_v2_dynamics_ctx_t *)user_data;
  const double x = vars[0];
  const double y = vars[1];
  const double z = vars[2];
  deriv[0] = y + 3.0 * x * x - x * x * x - ctx->vh * z + ctx->e - ctx->input_syn;
  deriv[1] = 1.0 - 5.0 * x * x - y;
  deriv[2] = ctx->mu * (-ctx->vh * z + ctx->s * (x + 1.6));
}

static void hindmarsh_rose_v2_init(hindmarsh_rose_v2_state_t *state) {
  if (state == NULL) {
    return;
  }
  state->x = -0.9013747551021072;
  state->y = -3.15948829665501;
  state->z = 3.247826955037619;
  state->input_syn = 0.0;
  state->e = 3.0;
  state->mu = 0.0021;
  state->s = 4.0;
  state->vh = 1.0;
  state->dt = 0.0015;
  state->burst_duration = 1.0;
  state->period_seconds = 0.001;
  state->s_points = 1;
  state->cfg_x = state->x;
  state->cfg_y = state->y;
  state->cfg_z = state->z;
}

hindmarsh_rose_v2_state_t *hindmarsh_rose_v2_new(void) {
  hindmarsh_rose_v2_state_t *state =
      (hindmarsh_rose_v2_state_t *)calloc(1, sizeof(*state));
  if (state == NULL) {
    return NULL;
  }
  hindmarsh_rose_v2_init(state);
  return state;
}

void hindmarsh_rose_v2_free(hindmarsh_rose_v2_state_t *state) { free(state); }

void hindmarsh_rose_v2_set_config(hindmarsh_rose_v2_state_t *state,
                                    const char *key, size_t len, double value) {
  if (state == NULL || key == NULL) {
    return;
  }

  if (key_eq(key, len, "x")) {
    state->cfg_x = value;
    state->x = value;
  } else if (key_eq(key, len, "y")) {
    state->cfg_y = value;
    state->y = value;
  } else if (key_eq(key, len, "z")) {
    state->cfg_z = value;
    state->z = value;
  } else if (key_eq(key, len, "e")) {
    state->e = value;
  } else if (key_eq(key, len, "mu")) {
    state->mu = value;
  } else if (key_eq(key, len, "s")) {
    state->s = value;
  } else if (key_eq(key, len, "vh")) {
    state->vh = value;
  } else if (key_eq(key, len, "burst_duration")) {
    state->burst_duration = value;
  } else if (key_eq(key, len, "period_seconds")) {
    state->period_seconds = value;
  }
  hindmarsh_rose_v2_update_burst_settings(state);
}

void hindmarsh_rose_v2_set_input(hindmarsh_rose_v2_state_t *state,
                                   const char *name, size_t len, double value) {
  if (state == NULL || name == NULL) {
    return;
  }
  if (key_eq(name, len, "i_syn")) {
    state->input_syn = value;
  }
}

void hindmarsh_rose_v2_process(hindmarsh_rose_v2_state_t *state) {
  if (state == NULL) {
    return;
  }
  if (!isfinite(state->dt) || state->dt <= DBL_MIN) {
    state->dt = 0.0015;
  }
  if (!isfinite(state->x) || !isfinite(state->y) || !isfinite(state->z)) {
    state->x = state->cfg_x;
    state->y = state->cfg_y;
    state->z = state->cfg_z;
  }

  // Limit steps for real-time performance
  size_t steps = state->s_points;
  if (steps == 0) {
    steps = 1;
  }
  if (steps > 50) { // Hard limit for real-time performance
    steps = 50;
  }
  hindmarsh_rose_v2_dynamics_ctx_t ctx = {
      .input_syn = state->input_syn,
      .e = state->e,
      .mu = state->mu,
      .s = state->s,
      .vh = state->vh,
  };
  for (size_t step = 0; step < steps; ++step) {
    double vars[3] = {state->x, state->y, state->z};
    rtsyn_plugin_rk4_step_n(vars, 3, state->dt, hindmarsh_rose_v2_deriv, &ctx);
    state->x = vars[0];
    state->y = vars[1];
    state->z = vars[2];
  }
}

double hindmarsh_rose_v2_get_output(
    const hindmarsh_rose_v2_state_t *state, const char *name, size_t len) {
  if (state == NULL || name == NULL) {
    return 0.0;
  }
  if (key_eq(name, len, "x")) return state->x;
  if (key_eq(name, len, "y")) return state->y;
  if (key_eq(name, len, "z")) return state->z;
  if (key_eq(name, len, "Membrane potential (mV)")) return state->x * 1000.0;
  if (key_eq(name, len, "Membrane potential (V)")) return state->x;
  return 0.0;
}

static void hindmarsh_rose_v2_update_burst_settings(hindmarsh_rose_v2_state_t *state) {
  if (state == NULL) {
    return;
  }
  if (state->period_seconds <= 0.0) {
    state->s_points = 1;
    return;
  }

  // Calculate optimal dt using RTXI-compatible algorithm
  double pts_burst =
      hindmarsh_rose_v2_select_pts_burst(state->burst_duration, state->period_seconds);
  state->dt = hindmarsh_rose_v2_select_optimal_dt(pts_burst);
  if (!isfinite(state->dt) || state->dt <= DBL_MIN) {
    state->dt = 0.0015;
  }

  // For real-time performance, limit integration steps regardless of frequency
  if (state->burst_duration > 0.0) {
    // Use fixed steps for burst mode to ensure consistent performance
    state->s_points = 1;
  } else {
    // Limit steps to maintain real-time performance
    const size_t max_steps =
        10; // Maximum steps per tick for real-time performance
    double desired = state->period_seconds / state->dt;
    if (!isfinite(desired) || desired <= 0.0) {
      state->s_points = 1;
      return;
    }
    size_t desired_steps = (size_t)llround(desired);
    if (desired_steps == 0) {
      desired_steps = 1;
    }

    if (desired_steps > max_steps) {
      // Adapt dt to maintain step count instead of increasing steps
      state->dt = state->period_seconds / max_steps;
      state->s_points = max_steps;
    } else {
      state->s_points = desired_steps;
    }
  }
}

double hindmarsh_rose_v2_select_optimal_dt(double pts_match) {
  if (!isfinite(pts_match)) {
    return 0.0015;
  }
  const double dts[] = {0.0005, 0.001, 0.0015, 0.002, 0.003, 0.005, 0.01, 0.015, 0.02, 0.03, 0.05, 0.1};
  const double pts[] = {577638.0, 286092.5, 189687.0, 142001.8, 94527.4, 56664.4, 28313.6, 18381.1, 14223.2, 9497.0, 5716.9, 2829.7};
  const size_t n = sizeof(dts) / sizeof(dts[0]);
  
  size_t best_idx = 0;
  double best_diff = fabs(pts[0] - pts_match);
  
  for (size_t i = 1; i < n; i++) {
    double diff = fabs(pts[i] - pts_match);
    if (diff < best_diff) {
      best_diff = diff;
      best_idx = i;
    }
  }
  
  return dts[best_idx];
}

double hindmarsh_rose_v2_select_pts_burst(double burst_duration, double period_seconds) {
  if (!isfinite(burst_duration) || !isfinite(period_seconds) ||
      period_seconds <= 0.0) {
    return 1.0;
  }
  return burst_duration / period_seconds;
}

static int key_eq(const char *key, size_t len, const char *lit) {
  size_t lit_len = strlen(lit);
  return len == lit_len && memcmp(key, lit, lit_len) == 0;
}
