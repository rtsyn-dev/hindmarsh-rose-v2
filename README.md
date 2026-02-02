# Hindmarsh Rose v2 for RTSyn

Hindmarsh-Rose v2 plugin with C++.

## Requirements

### Rust toolchain (stable) with Cargo

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source "$HOME/.cargo/env"
```

### C/C++ toolchain

On Debian/Ubuntu:

```bash
sudo apt install build-essential g++
```

On Fedora/RHEL/CentOS:

```bash
sudo dnf install gcc gcc-c++ make
```

On Arch:

```bash
sudo pacman -Syu base-devel gcc
```

## Usage

Import this plugin in RTSyn from the plugin manager/installer, add it to the runtime, connect its ports, and start it from the plugin controls.
