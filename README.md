# Hindmarsh Rose v2 for RTSyn

Hindmarsh-Rose v2 neuron model with C implementation

## Requirements

### Rust toolchain (stable) with Cargo

```bash
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source "$HOME/.cargo/env"
```

### C toolchain

On Debian/Ubuntu:

```bash
sudo apt install build-essential gcc
```

On Fedora/RHEL/CentOS:

```bash
sudo dnf install gcc make
```

On Arch:

```bash
sudo pacman -Syu base-devel gcc
```

## Usage

Import this plugin in RTSyn from the plugin manager/installer, add it to the runtime, connect its ports, and start it from the plugin controls.
