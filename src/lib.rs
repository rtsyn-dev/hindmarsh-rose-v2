use rtsyn_plugin::prelude::*;
use serde_json::Value;

struct HindmarshRoseV2 {
    x: f64,
    y: f64,
    z: f64,
    input_syn: f64,
    e: f64,
    mu: f64,
    s: f64,
    vh: f64,
    dt: f64,
    burst_duration: f64,
    s_points: usize,
    period_seconds: f64,
    cfg_x: f64,
    cfg_y: f64,
    cfg_z: f64,
}

impl Default for HindmarshRoseV2 {
    fn default() -> Self {
        let x = -0.9013747551021072;
        let y = -3.15948829665501;
        let z = 3.247826955037619;
        Self {
            x,
            y,
            z,
            input_syn: 0.0,
            e: 3.0,
            mu: 0.0021,
            s: 4.0,
            vh: 1.0,
            dt: 0.0015,
            burst_duration: 1.0,
            s_points: 1,
            period_seconds: 0.001,
            cfg_x: x,
            cfg_y: y,
            cfg_z: z,
        }
    }
}

impl HindmarshRoseV2 {
    fn update_burst_settings(&mut self) {
        if self.period_seconds <= 0.0 {
            self.s_points = 1;
            return;
        }

        if self.burst_duration > 0.0 {
            let freq = 1.0 / self.period_seconds;
            let pts_match = self.burst_duration * freq;
            self.dt = self.select_optimal_dt(pts_match);
            self.s_points = ((self.select_pts_burst(self.burst_duration, freq)
                / (self.burst_duration * freq))
                .round() as usize)
                .max(1);
        } else {
            let steps = ((self.period_seconds / self.dt).round() as usize).max(1);
            self.s_points = steps;
        }

        if self.s_points == 0 {
            self.s_points = 1;
        }
    }

    fn select_optimal_dt(&self, pts_match: f64) -> f64 {
        let dts = [
            0.0005, 0.001, 0.0015, 0.002, 0.003, 0.005, 0.01, 0.015, 0.02, 0.03, 0.05, 0.1,
        ];
        let pts = [
            577638.0, 286092.5, 189687.0, 142001.8, 94527.4, 56664.4, 28313.6, 18381.1, 14223.2,
            9497.0, 5716.9, 2829.7,
        ];

        for (i, &pt) in pts.iter().enumerate() {
            if pt <= pts_match {
                return dts[i];
            }
        }
        dts[dts.len() - 1]
    }

    fn select_pts_burst(&self, sec_per_burst: f64, freq: f64) -> f64 {
        let pts_match = sec_per_burst * freq;
        self.select_optimal_dt(pts_match);
        pts_match
    }

    fn step(&mut self) {
        let dt = self.dt;
        let e = self.e;
        let mu = self.mu;
        let s = self.s;
        let vh = self.vh;
        let input_syn = self.input_syn;

        let mut vars = [self.x, self.y, self.z];
        rk4_step(&mut vars, dt, |state, deriv| {
            let x = state[0];
            let y = state[1];
            let z = state[2];
            deriv[0] = y + 3.0 * (x * x) - (x * x * x) - vh * z + e - input_syn;
            deriv[1] = 1.0 - 5.0 * (x * x) - y;
            deriv[2] = mu * (-vh * z + s * (x + 1.6));
        });

        self.x = vars[0];
        self.y = vars[1];
        self.z = vars[2];
    }
}

impl PluginDescriptor for HindmarshRoseV2 {
    fn name() -> &'static str {
        "Hindmarsh Rose v2"
    }

    fn kind() -> &'static str {
        "hindmarsh_rose_v2"
    }

    fn plugin_type() -> PluginType {
        PluginType::Computational
    }

    fn inputs() -> &'static [&'static str] {
        &["i_syn"]
    }

    fn outputs() -> &'static [&'static str] {
        &["Membrane potential (V)", "Membrane potential (mV)"]
    }

    fn internal_variables() -> &'static [&'static str] {
        &["x", "y", "z"]
    }

    fn default_vars() -> Vec<(&'static str, Value)> {
        vec![
            ("x", (-0.9013747551021072).into()),
            ("y", (-3.15948829665501).into()),
            ("z", 3.247826955037619.into()),
            ("e", 3.0.into()),
            ("mu", 0.0021.into()),
            ("s", 4.0.into()),
            ("vh", 1.0.into()),
            ("dt", 0.0015.into()),
            ("burst_duration", 1.0.into()),
        ]
    }

    fn behavior() -> PluginBehavior {
        PluginBehavior {
            loads_started: false,
            ..PluginBehavior::default()
        }
    }
}

impl PluginRuntime for HindmarshRoseV2 {
    fn set_config_value(&mut self, key: &str, value: &Value) {
        if let Some(v) = value.as_f64() {
            match key {
                "x" => self.cfg_x = v,
                "y" => self.cfg_y = v,
                "z" => self.cfg_z = v,
                "e" => self.e = v,
                "mu" => self.mu = v,
                "s" => self.s = v,
                "vh" => self.vh = v,
                "dt" => self.dt = v,
                "burst_duration" => self.burst_duration = v,
                "period_seconds" => self.period_seconds = v,
                _ => {}
            }
        }

        if (self.x, self.y, self.z) != (self.cfg_x, self.cfg_y, self.cfg_z) {
            self.x = self.cfg_x;
            self.y = self.cfg_y;
            self.z = self.cfg_z;
        }

        self.update_burst_settings();
    }

    fn set_input_value(&mut self, key: &str, value: f64) {
        if key == "i_syn" {
            self.input_syn = if value.is_finite() { value } else { 0.0 };
        }
    }

    fn process_tick(&mut self, _tick: u64, period_seconds: f64) {
        if (self.period_seconds - period_seconds).abs() > f64::EPSILON {
            self.period_seconds = period_seconds;
            self.update_burst_settings();
        }

        let steps = self.s_points.min(10_000).max(1);
        for _ in 0..steps {
            self.step();
        }
    }

    fn get_output_value(&self, key: &str) -> f64 {
        match key {
            "Membrane potential (V)" => self.x,
            "Membrane potential (mV)" => self.x * 1000.0,
            _ => 0.0,
        }
    }

    fn get_internal_value(&self, key: &str) -> Option<f64> {
        match key {
            "x" => Some(self.x),
            "y" => Some(self.y),
            "z" => Some(self.z),
            _ => None,
        }
    }
}

rtsyn_plugin::export_plugin!(HindmarshRoseV2);
