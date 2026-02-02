use rtsyn_plugin::prelude::*;
use serde_json::Value;
use std::ffi::c_void;

#[repr(C)]
struct HindmarshRoseV2Core(c_void);

extern "C" {
    fn hindmarsh_rose_v2_new() -> *mut HindmarshRoseV2Core;
    fn hindmarsh_rose_v2_free(state: *mut HindmarshRoseV2Core);
    fn hindmarsh_rose_v2_set_config(
        state: *mut HindmarshRoseV2Core,
        key: *const u8,
        len: usize,
        value: f64,
    );
    fn hindmarsh_rose_v2_set_input(
        state: *mut HindmarshRoseV2Core,
        name: *const u8,
        len: usize,
        value: f64,
    );
    fn hindmarsh_rose_v2_process(state: *mut HindmarshRoseV2Core);
    fn hindmarsh_rose_v2_get_output(
        state: *const HindmarshRoseV2Core,
        name: *const u8,
        len: usize,
    ) -> f64;
}

struct HindmarshRoseV2 {
    state: *mut HindmarshRoseV2Core,
}

impl Default for HindmarshRoseV2 {
    fn default() -> Self {
        let state = unsafe { hindmarsh_rose_v2_new() };
        Self { state }
    }
}

impl Drop for HindmarshRoseV2 {
    fn drop(&mut self) {
        if !self.state.is_null() {
            unsafe { hindmarsh_rose_v2_free(self.state) };
            self.state = std::ptr::null_mut();
        }
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
        if self.state.is_null() {
            return;
        }
        if let Some(v) = value.as_f64() {
            unsafe { hindmarsh_rose_v2_set_config(self.state, key.as_ptr(), key.len(), v) };
        }
    }

    fn set_input_value(&mut self, key: &str, value: f64) {
        if self.state.is_null() {
            return;
        }
        let v = if value.is_finite() { value } else { 0.0 };
        unsafe { hindmarsh_rose_v2_set_input(self.state, key.as_ptr(), key.len(), v) };
    }

    fn process_tick(&mut self, _tick: u64, period_seconds: f64) {
        if self.state.is_null() {
            return;
        }
        unsafe {
            hindmarsh_rose_v2_set_config(
                self.state,
                b"period_seconds".as_ptr(),
                b"period_seconds".len(),
                period_seconds,
            );
            hindmarsh_rose_v2_process(self.state);
        }
    }

    fn get_output_value(&self, key: &str) -> f64 {
        if self.state.is_null() {
            return 0.0;
        }
        unsafe { hindmarsh_rose_v2_get_output(self.state, key.as_ptr(), key.len()) }
    }

    fn get_internal_value(&self, key: &str) -> Option<f64> {
        if self.state.is_null() {
            return None;
        }
        match key {
            "x" | "y" | "z" => Some(unsafe {
                hindmarsh_rose_v2_get_output(self.state, key.as_ptr(), key.len())
            }),
            _ => None,
        }
    }
}

rtsyn_plugin::export_plugin!(HindmarshRoseV2);
