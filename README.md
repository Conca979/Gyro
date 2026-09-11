# Gyro 🌀

Real-time 3D geometric visualization and ASCII rendering in pure C.

`Gyro` is a lightweight, zero-dependency terminal graphics playground. It uses custom z-buffering, 3D rotational projection mathematics, and surface normal illumination to render interactive, rotating objects directly inside your terminal window.

---

## 🎬 Demos

### ☀️ Solar System & Planetary Orbits (`src/spining_solar.c`)
Real-time celestial simulation with a glowing Sun, orbiting planets (Earth, Mars, Jupiter, Saturn with 3D tilted rings, and Moon), dynamic radial point-lighting (day/night shading), and planet camera targeting.

![Solar System Demo](gifs/solar_demo.gif)

---

### 🤖 3D Moving Body & Third-Person Camera (`src/moving_body.c`)
Hierarchically animated robot with smooth walking limb motions, a spatial reference floor grid, full third-person camera orbiting, and WASD movement navigation.

![Moving Body Demo](gifs/body_demo.gif)

---

### 🧬 DNA Double Helix (Interactive) (`src/spining_dna.c`)
Parametric double helix visualization with real-time interactive camera controls and base-pair rungs.

![DNA Double Helix Demo](gifs/dna_demo.gif)

---

### 🍩 3D Illuminated Donut (`src/spining_donus.c`)
Real-time surface normal illumination with 12-level ASCII shading.

![3D Donut Demo](gifs/donut_demo.gif)

---

### 🧊 Triple Rotating Cubes (`src/spining_cube.c`)
Multi-cube projection with independent spatial offsets, depth sorting, and real-time 3D rotation controls.

![3D Cube Demo](gifs/cube_demo.gif)

---

## Features

- **Solar System & Planetary Orbits** (`src/spining_solar.c`): Multi-body celestial system with central radial point lighting (day/night hemisphere shading), hierarchical moon orbits, Saturn's ring disc, time speed scaling, and planet camera focus switching.
- **3D Moving Body & Camera** (`src/moving_body.c`): A hierarchically animated robot with smooth limb walking animations, set in a true 3D space with a third-person camera. Control movement with `WASD` and orbit the camera with `IJKL`.
- **DNA Double Helix** (`src/spining_dna.c`): Parametric double helix visualization with real-time interactive camera controls.
- **3D Torus (Donut)** (`src/spining_donus.c`): Real-time surface normal lighting mapped to an ASCII luminance ramp (`.,-~:;=!*#$@`).
- **Triple Rotating Cubes** (`src/spining_cube.c`): Three simultaneous cubes of varying sizes with independent spatial offsets.
- **Depth Buffering**: Custom z-buffer to prevent surface overlap and clipping artifacts.
- **Pure C**: Zero external graphic libraries or dependencies, relying only on standard C runtime (`math.h`, `stdio.h`, `string.h`, `unistd.h`).

---

## 🚀 Quick Start

### Prerequisites

A C compiler such as `gcc` or `clang` (e.g. MinGW / MSYS2 on Windows, or standard GCC on Linux / macOS).

---

### 1. ☀️ Solar System & Planetary Orbits

```bash
# Compile
gcc src/spining_solar.c -o run -lm

# Run (Windows)
.\run.exe

# Run (Linux / macOS)
./run
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
# Compile
gcc src/moving_body.c -o run -lm

# Run (Windows)
.\run.exe

# Run (Linux / macOS)
./run
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
# Compile
gcc src/spining_dna.c -o run -lm

# Run (Windows)
.\run.exe

# Run (Linux / macOS)
./run
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
# Compile
gcc src/spining_cube.c -o run -lm

# Run (Windows)
.\run.exe

# Run (Linux / macOS)
./run
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
# Compile
gcc src/spining_donus.c -o run -lm

# Run (Windows)
.\run.exe

# Run (Linux / macOS)
./run
```

*Continuous autonomous dual-axis rotation with dynamic 12-level surface normal luminance calculation.*

---

> **Tip**: For the best visual experience, set your terminal window to at least **160 × 44 characters** (or maximize / fullscreen the window).

---

## Roadmap

See [FUTURE_TASKS.md](FUTURE_TASKS.md) for detailed mathematical specifications, controls, and planned simulations.

- [x] 3D Animated Hierarchical Body (`src/moving_body.c`)
- [x] DNA Double Helix visualization (`src/spining_dna.c`)
- [x] Interactive rotation and camera controls (`WASD` / `IJKL` / zoom)
- [x] Solar System & Planetary Orbits (`src/spining_solar.c`)
- [ ] Procedural 3D Terrain Flight Simulator (`src/fly_terrain.c`)
- [ ] Black Hole & Gravitational Lensing (`src/black_hole.c`)
- [ ] Interactive 3D Rubik's Cube (`src/rubiks_cube.c`)
- [ ] 3D Wave Tank & Ripple Surface (`src/wave_surface.c`)
- [ ] Jumping & Gravity mechanics in `src/moving_body.c`
- [ ] ANSI TrueColor (24-bit RGB) shading support
