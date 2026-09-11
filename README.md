# Gyro 🌀

Real-time 3D geometric visualization and ASCII rendering in pure C.

`Gyro` is a lightweight, zero-dependency terminal graphics playground. It uses custom z-buffering, 3D rotational projection mathematics, and surface normal illumination to render interactive, rotating objects directly inside your terminal window.

---

## 🎬 Demos

### 🧬 DNA Double Helix (Interactive) (`src/spining_dna.c`)
Parametric double helix visualization with real-time interactive camera controls.

![DNA Double Helix Demo](gifs/dna_demo.gif)

---

### 🍩 3D Illuminated Donut (`src/spining_donus.c`)
Real-time surface normal illumination with 12-level ASCII shading.

![3D Donut Demo](gifs/donut_demo.gif)

---

### 🧊 Triple Rotating Cubes (`src/spining_cube.c`)
Multi-cube projection with independent spatial offsets and depth sorting.

![3D Cube Demo](gifs/cube_demo.gif)

> *(To add or update recordings, place your `.gif` files in the `gifs/` directory with the names `dna_demo.gif`, `donut_demo.gif` and `cube_demo.gif`)*

---

## Features

- **DNA Double Helix** (`src/spining_dna.c`): Parametric double helix visualization with real-time interactive camera controls (`WASD` for rotation, `+/-` for zoom).
- **3D Torus (Donut)** (`src/spining_donus.c`): Real-time surface normal lighting mapped to an ASCII luminance ramp (`.,-~:;=!*#$@`).
- **Triple Rotating Cubes** (`src/spining_cube.c`): Three simultaneous cubes of varying sizes with independent spatial offsets.
- **Depth Buffering**: Custom z-buffer to prevent surface overlap and clipping artifacts.
- **Pure C**: Zero external graphic libraries or dependencies, relying only on standard C runtime (`math.h`, `stdio.h`, `string.h`, `unistd.h`).

---

## 🚀 Quick Start

### Prerequisites

A C compiler such as `gcc` or `clang` (e.g. MinGW / MSYS2 on Windows, or standard GCC on Linux / macOS).

### 1. DNA Double Helix (Interactive)

```bash
# Compile
gcc src/spining_dna.c -o run -lm

# Run (Windows)
.\run.exe

# Run (Linux / macOS)
./run
```

### 2. 3D Spinning Donut

```bash
# Compile
gcc src/spining_donus.c -o run -lm

# Run (Windows)
.\run.exe

# Run (Linux / macOS)
./run
```

### 3. Triple Spinning Cubes

```bash
# Compile
gcc src/spining_cube.c -o run -lm

# Run (Windows)
.\run.exe

# Run (Linux / macOS)
./run
```

> **Tip**: For the best visual experience, set your terminal window to at least **160 × 44 characters** (or maximize / fullscreen the window).

---

## Roadmap

- [ ] 3D Wireframe renderer
- [x] DNA Double Helix visualization
- [ ] ANSI TrueColor (24-bit RGB) shading support
- [x] Interactive rotation and camera controls (`WASD` / zoom)
