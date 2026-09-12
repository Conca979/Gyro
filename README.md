# Gyro 🌀

Real-time 3D geometric visualization and ASCII rendering in pure C.

`Gyro` is a lightweight, zero-dependency terminal graphics playground. It uses custom z-buffering, 3D rotational projection mathematics, and surface normal illumination to render interactive, rotating objects directly inside your terminal window.

---

## 🎬 Demos

### ☀️ Solar System & Planetary Orbits (`src/solar_spinning.c`)
Real-time celestial simulation with a glowing Sun, orbiting planets (Earth, Mars, Jupiter, Saturn with 3D tilted rings, and Moon), dynamic radial point-lighting (day/night shading), and planet camera targeting.

![Solar System Demo](gifs/solar_demo.gif)

---

### 🤖 3D Moving Body & Third-Person Camera (`src/body_moving.c`)
Hierarchically animated robot with smooth walking limb motions, a spatial reference floor grid, full third-person camera orbiting, and WASD movement navigation.

![Moving Body Demo](gifs/body_demo.gif)

---

### 🧬 DNA Double Helix (Interactive) (`src/dna_spinning.c`)
Parametric double helix visualization with real-time interactive camera controls and base-pair rungs.

![DNA Double Helix Demo](gifs/dna_demo.gif)

---

### 🍩 3D Illuminated Donut (`src/donut_spinning.c`)
Real-time surface normal illumination with 12-level ASCII shading.

![3D Donut Demo](gifs/donut_demo.gif)

---

### 🧊 Triple Rotating Cubes (`src/cube_spinning.c`)
Multi-cube projection with independent spatial offsets, depth sorting, and real-time 3D rotation controls.

![3D Cube Demo](gifs/cube_demo.gif)

---

## Features

- **Solar System & Planetary Orbits** (`src/solar_spinning.c`): Multi-body celestial system with central radial point lighting (day/night hemisphere shading), hierarchical moon orbits, Saturn's ring disc, time speed scaling, and planet camera focus switching.
- **3D Moving Body & Camera** (`src/body_moving.c`): A hierarchically animated robot with smooth limb walking animations, set in a true 3D space with a third-person camera. Control movement with `WASD` and orbit the camera with `IJKL`.
- **DNA Double Helix** (`src/dna_spinning.c`): Parametric double helix visualization with real-time interactive camera controls.
- **3D Torus (Donut)** (`src/donut_spinning.c`): Real-time surface normal lighting mapped to an ASCII luminance ramp (`.,-~:;=!*#$@`).
- **Triple Rotating Cubes** (`src/cube_spinning.c`): Three simultaneous cubes of varying sizes with independent spatial offsets.
- **Depth Buffering**: Custom z-buffer to prevent surface overlap and clipping artifacts.
- **Pure C**: Zero external graphic libraries or dependencies, relying only on standard C runtime (`math.h`, `stdio.h`, `string.h`, `unistd.h`).

---

## 🚀 Quick Start

### Prerequisites

A C compiler such as `gcc` or `clang` (e.g. MinGW / MSYS2 on Windows, or standard GCC on Linux / macOS), and `make`.

### Build with Make (Recommended)

Build all demos into the `bin/` directory at once:

```bash
make
```

Or build an individual demo:

```bash
make solar_spinning
make body_moving
make dna_spinning
make cube_spinning
make donut_spinning
```

Clean all compiled executables:

```bash
make clean
```

All compiled binaries are placed in `bin/` with identical base names (`bin/<name>.exe` on Windows, `bin/<name>` on Linux/macOS).

---

### 1. ☀️ Solar System & Planetary Orbits

```bash
# Build
make solar_spinning

# Or compile manually with gcc:
# gcc src/solar_spinning.c -o bin/solar_spinning -lm

# Run (Windows)
.\bin\solar_spinning.exe

# Run (Linux / macOS)
./bin/solar_spinning
```

**Interactive Controls:**
| Key | Action |
| :--- | :--- |
| `W` / `A` / `S` / `D` | Fly forward / left / backward / right (relative to camera direction) |
| `Q` / `E` | Fly straight up / down |
| `I` / `J` / `K` / `L` | Look around (pitch up/down, yaw left/right) |
| `[` / `]` | Halve / double simulation time speed |
| `P` | Pause / resume orbits |
| `Esc` | Exit |

---

### 2. 🤖 3D Moving Body & Camera

```bash
# Build
make body_moving

# Or compile manually with gcc:
# gcc src/body_moving.c -o bin/body_moving -lm

# Run (Windows)
.\bin\body_moving.exe

# Run (Linux / macOS)
./bin/body_moving
```

**Interactive Controls:**
| Key | Action |
| :--- | :--- |
| `W` / `A` / `S` / `D` | Walk forward / step left / backward / step right (relative to camera view) |
| `I` / `J` / `K` / `L` | Orbit third-person camera around the robot |
| `+` / `-` | Zoom in / out |
| `Esc` | Exit |

---

### 3. 🧬 DNA Double Helix (Interactive)

```bash
# Build
make dna_spinning

# Or compile manually with gcc:
# gcc src/dna_spinning.c -o bin/dna_spinning -lm

# Run (Windows)
.\bin\dna_spinning.exe

# Run (Linux / macOS)
./bin/dna_spinning
```

**Interactive Controls:**
| Key | Action |
| :--- | :--- |
| `W` / `S` | Pitch rotation (tilt up / down) |
| `A` / `D` | Yaw rotation (turn left / right) |
| `Q` / `E` | Roll rotation |
| `+` / `-` | Zoom in / out |
| `Space` | Pause / resume auto-rotation |
| `Esc` | Exit |

---

### 4. 🧊 Triple Rotating Cubes

```bash
# Build
make cube_spinning

# Or compile manually with gcc:
# gcc src/cube_spinning.c -o bin/cube_spinning -lm

# Run (Windows)
.\bin\cube_spinning.exe

# Run (Linux / macOS)
./bin/cube_spinning
```

**Interactive Controls:**
| Key | Action |
| :--- | :--- |
| `W` / `S` | Pitch rotation |
| `A` / `D` | Yaw rotation |
| `Q` / `E` | Roll rotation |
| `+` / `-` | Zoom in / out |
| `Space` | Pause / resume auto-rotation |
| `Esc` | Exit |

---

### 5. 🍩 3D Illuminated Donut

```bash
# Build
make donut_spinning

# Or compile manually with gcc:
# gcc src/donut_spinning.c -o bin/donut_spinning -lm

# Run (Windows)
.\bin\donut_spinning.exe

# Run (Linux / macOS)
./bin/donut_spinning
```

*Continuous autonomous dual-axis rotation with dynamic 12-level surface normal luminance calculation.*

---

> **Tip**: For the best visual experience, set your terminal window to at least **160 × 44 characters** (or maximize / fullscreen the window).

---

## Roadmap

See [FUTURE_TASKS.md](FUTURE_TASKS.md) for detailed mathematical specifications, controls, and planned simulations.

- [x] 3D Animated Hierarchical Body (`src/body_moving.c`)
- [x] DNA Double Helix visualization (`src/dna_spinning.c`)
- [x] Interactive rotation and camera controls (`WASD` / `IJKL` / zoom)
- [x] Solar System & Planetary Orbits (`src/solar_spinning.c`)
- [ ] Procedural 3D Terrain Flight Simulator (`src/fly_terrain.c`)
- [ ] Black Hole & Gravitational Lensing (`src/black_hole.c`)
- [ ] Interactive 3D Rubik's Cube (`src/rubiks_cube.c`)
- [ ] 3D Wave Tank & Ripple Surface (`src/wave_surface.c`)
- [ ] Jumping & Gravity mechanics in `src/body_moving.c`
- [ ] ANSI TrueColor (24-bit RGB) shading support
