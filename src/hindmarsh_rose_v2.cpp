#include "hindmarsh_rose_v2.h"
#include <cmath>
#include <cstdlib>
#include <cstring>

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
typedef void (*rk4_deriv_fn_t)(const double *state, double *deriv, size_t n,
                               void *user_data);
extern "C" void rtsyn_plugin_rk4_step_n(double *state, size_t n, double dt,
                                         rk4_deriv_fn_t deriv_fn,
                                         void *user_data);

struct hindmarsh_rose_v2_dynamics_ctx_t {
  double input_syn;
  double e;
  double mu;
  double s;
  double vh;
};

extern "C" void hindmarsh_rose_v2_deriv(const double *vars, double *deriv,
                                             size_t n, void *user_data) {
  (void)n;
  if (vars == nullptr || deriv == nullptr || user_data == nullptr) {
    return;
  }
  const auto *ctx =
      static_cast<const hindmarsh_rose_v2_dynamics_ctx_t *>(user_data);
  const double x = vars[0];
  const double y = vars[1];
  const double z = vars[2];
  deriv[0] = y + 3.0 * x * x - x * x * x - ctx->vh * z + ctx->e - ctx->input_syn;
  deriv[1] = 1.0 - 5.0 * x * x - y;
  deriv[2] = ctx->mu * (-ctx->vh * z + ctx->s * (x + 1.6));
}

static void hindmarsh_rose_v2_init(hindmarsh_rose_v2_state_t *state) {
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

extern "C" hindmarsh_rose_v2_state_t *hindmarsh_rose_v2_new() {
  auto *state = static_cast<hindmarsh_rose_v2_state_t *>(
      std::calloc(1, sizeof(hindmarsh_rose_v2_state_t)));
  if (state == nullptr) {
    return nullptr;
  }
  hindmarsh_rose_v2_init(state);
  return state;
}

extern "C" void hindmarsh_rose_v2_free(
    hindmarsh_rose_v2_state_t *state) {
  std::free(state);
}

extern "C" void hindmarsh_rose_v2_set_config(
    hindmarsh_rose_v2_state_t *state, const char *key, size_t len,
    double value) {
  if (len == 1 && strncmp(key, "x", len) == 0) {
    state->cfg_x = value;
    state->x = value;
  } else if (len == 1 && strncmp(key, "y", len) == 0) {
    state->cfg_y = value;
    state->y = value;
  } else if (len == 1 && strncmp(key, "z", len) == 0) {
    state->cfg_z = value;
    state->z = value;
  } else if (len == 1 && strncmp(key, "e", len) == 0) {
    state->e = value;
  } else if (len == 2 && strncmp(key, "mu", len) == 0) {
    state->mu = value;
  } else if (len == 1 && strncmp(key, "s", len) == 0) {
    state->s = value;
  } else if (len == 2 && strncmp(key, "vh", len) == 0) {
    state->vh = value;
  } else if (len == 14 && strncmp(key, "burst_duration", len) == 0) {
    state->burst_duration = value;
  } else if (len == 14 && strncmp(key, "period_seconds", len) == 0) {
    state->period_seconds = value;
  }
  hindmarsh_rose_v2_update_burst_settings(state);
}

extern "C" void hindmarsh_rose_v2_set_input(
    hindmarsh_rose_v2_state_t *state, const char *name, size_t len,
    double value) {
  if (len == 5 && strncmp(name, "i_syn", len) == 0) {
    state->input_syn = value;
  }
}

extern "C" void hindmarsh_rose_v2_process(
    hindmarsh_rose_v2_state_t *state) {
  // Limit steps for real-time performance
  size_t steps = state->s_points;
  if (steps == 0) {
    steps = 1;
  }
  if (steps > 50) { // Hard limit for real-time performance
    steps = 50;
  }
  hindmarsh_rose_v2_dynamics_ctx_t ctx = {
      state->input_syn, state->e, state->mu, state->s, state->vh};
  for (size_t step = 0; step < steps; ++step) {
    double vars[3] = {state->x, state->y, state->z};
    rtsyn_plugin_rk4_step_n(vars, 3, state->dt, hindmarsh_rose_v2_deriv,
                            &ctx);
    state->x = vars[0];
    state->y = vars[1];
    state->z = vars[2];
  }
}

extern "C" double hindmarsh_rose_v2_get_output(
    const hindmarsh_rose_v2_state_t *state, const char *name, size_t len) {
  if (state == nullptr || name == nullptr) {
    return 0.0;
  }
  if (len == 1 && std::strncmp(name, "x", len) == 0) return state->x;
  if (len == 1 && std::strncmp(name, "y", len) == 0) return state->y;
  if (len == 1 && std::strncmp(name, "z", len) == 0) return state->z;
  if (len == 22 && std::strncmp(name, "Membrane potential (V)", len) == 0) return state->x;
  if (len == 23 && std::strncmp(name, "Membrane potential (mV)", len) == 0) return state->x * 1000.0;
  return 0.0;
}

static void hindmarsh_rose_v2_update_burst_settings(hindmarsh_rose_v2_state_t *state) {
  if (state->period_seconds <= 0.0) {
    state->s_points = 1;
    return;
  }

  // Calculate optimal dt using RTXI-compatible algorithm
  double pts_burst = hindmarsh_rose_v2_select_pts_burst(state->burst_duration, state->period_seconds);
  state->dt = hindmarsh_rose_v2_select_optimal_dt(pts_burst);

  // For real-time performance, limit integration steps regardless of frequency
  if (state->burst_duration > 0.0) {
    // Use fixed steps for burst mode to ensure consistent performance
    state->s_points = 1;
  } else {
    // Limit steps to maintain real-time performance
    const size_t max_steps =
        10; // Maximum steps per tick for real-time performance
    size_t desired_steps = (size_t)llround(state->period_seconds / state->dt);
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
  const double dts[] = {0.0005, 0.001, 0.0015, 0.002, 0.003, 0.005, 0.01, 0.015, 0.02, 0.03, 0.05, 0.1};
  const double pts[] = {577638.0, 286092.5, 189687.0, 142001.8, 94527.4, 56664.4, 28313.6, 18381.1, 14223.2, 9497.0, 5716.9, 2829.7};
  const size_t n = sizeof(dts) / sizeof(dts[0]);
  
  size_t best_idx = 0;
  double best_diff = std::abs(pts[0] - pts_match);
  
  for (size_t i = 1; i < n; i++) {
    double diff = std::abs(pts[i] - pts_match);
    if (diff < best_diff) {
      best_diff = diff;
      best_idx = i;
    }
  }
  
  return dts[best_idx];
}

double hindmarsh_rose_v2_select_pts_burst(double burst_duration, double period_seconds) {
  if (period_seconds <= 0.0) return 1.0;
  return burst_duration / period_seconds;
}
