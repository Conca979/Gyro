# Gyro 🌀 – Future Tasks & Roadmap

This document outlines planned 3D objects, physics expansions, and interactive simulations for the `Gyro` terminal graphics engine.

---

## 🌌 Category 1: New 3D Objects & Mathematical Worlds

- [x] **Solar System & Planetary Orbits (`src/solar_spinning.c`)**
  - **Description**: Real-time simulation of our solar system with a central illuminated Sun, orbiting planets, and moons.
  - **Mathematics**:
    - Circular & Keplerian elliptical orbital paths: `x(t) = a * cos(ω * t)`, `z(t) = b * sin(ω * t)`.
    - Sphere rendering with surface normal illumination and planetary axial tilts.
    - Saturn with 3D tilted concentric ring discs (`r_inner < r < r_outer`).
  - **Interactions**:
    - Number keys `1`–`9` to lock camera focus onto specific celestial bodies (Sun, Earth, Saturn, etc.).
    - `[` / `]` to speed up or slow down orbital time; `P` to pause/resume.
    - Camera orbit (`I` / `J` / `K` / `L`) and zoom (`+` / `-`).

- [ ] **Procedural 3D Terrain Flight (`src/fly_terrain.c`)**
  - **Description**: An endless flight simulator traversing procedurally generated rolling mountains and valleys.
  - **Mathematics**:
    - Multi-octave sinusoidal / Perlin synthesis: `h(x, z) = Σ A_i * sin(ω_i_x * x + ω_i_z * z)`.
    - Perspective projection grid with continuous z-axis scrolling.
    - Elevation-based ASCII shading palette: `.` (valleys) -> `,` -> `-` -> `~` -> `=` -> `*` -> `#` -> `@` (snow peaks).
  - **Interactions**:
    - `W` / `S` for pitch (nose down / up).
    - `A` / `D` for roll (bank left / right).
    - `Q` / `E` for yaw (rudder turn).
    - `+` / `-` to adjust flight velocity.

- [ ] **Black Hole & Gravitational Lensing (`src/black_hole.c`)**
  - **Description**: An ASCII visualization of a Schwarzschild black hole with an accretion disk and gravitational light deflection.
  - **Mathematics**:
    - Curved light ray paths bent around the event horizon: light deflection angle `α ≈ 4 * G * M / (c^2 * b)`.
    - Relativistic Doppler beaming: approaching side of the accretion disk appears brighter and blue-shifted (mapped to denser ASCII glyphs like `@`, `#`).
    - Event horizon shadow cast in the center.
  - **Interactions**:
    - Camera orbit around the singularity (`I` / `J` / `K` / `L`).
    - Spin rate adjustments and zoom controls.

- [x] **Interactive 3D Rubik's Cube (`src/rubiks_cube.c`)**
  - **Description**: Fully functional 3×3×3 Rubik's cube rendered with 27 sub-cubes.
  - **Mathematics**:
    - Dynamic hierarchical transformations: rotating a slice involves transforming 9 sub-cubes by 90° about a chosen principal axis (X, Y, or Z).
    - Distinct face stickers rendered with terminal glyph patterns or ANSI colors.
  - **Interactions**:
    - Standard Singmaster notation keybindings (`U`, `D`, `L`, `R`, `F`, `B` for clockwise; `Shift` + key for counter-clockwise).
    - Smooth 90-degree twist animation interpolation.
    - Free camera orbit to inspect all sides.

- [ ] **3D Wave Tank & Liquid Ripples (`src/wave_surface.c`)**
  - **Description**: Interactive water surface simulation reacting in real-time to disturbances.
  - **Mathematics**:
    - Discrete 2D wave equation: `d²u/dt² = c² * ∇²u - γ * (du/dt)`.
    - Projected 3D surface mesh with depth-buffering and specular surface reflection calculation.
  - **Interactions**:
    - `Spacebar` / Click: Drop a pebble into the tank to trigger dispersing concentric ripples.
    - `[` / `]` to change water damping and viscosity `γ`.
    - `WASD` / `IJKL` to orbit and tilt the tank.

---

## 🤖 Category 2: Moving Body & World Physics Upgrades (`src/body_moving.c`)

- [ ] **Jumping & Gravity Mechanics**
  - **Mechanics**:
    - Vertical velocity `vy` and gravitational acceleration `g` (e.g. `g = 9.8 m/s²`).
    - `Spacebar` triggers an impulse: `vy = v_jump`.
    - Mid-air limb animation blending (tucking legs / reaching arms during jump and landing).

- [ ] **3D Obstacles & Collision System**
  - **Mechanics**:
    - Axis-Aligned Bounding Box (AABB) collisions against static platforms, pillars, and stairs.
    - Stepping mechanic: automatically step up obstacles below a threshold height (e.g. `0.3` units).

- [ ] **Dynamic Point Light / Lantern**
  - **Mechanics**:
    - Moving light source anchored to the robot (or orbiting separately).
    - Distance-attenuated illumination: `I = I_0 / (1.0 + k_1 * d + k_2 * d²)`.
    - Casts dynamic highlights and shadows onto the floor grid and surrounding geometry.

- [ ] **Camera Mode Toggle (`V` key)**
  - **Mechanics**:
    - Press `V` to toggle between:
      1. **Third-Person View (TPV)**: Orbiting camera centered on the robot.
      2. **First-Person View (FPV)**: Camera placed directly at the robot's head/visor position, moving and rotating with the robot's head.

---

## 🎮 Category 3: Terminal 3D Mini-Games

- [ ] **3D Wireframe Trench Run (`src/trench_run.c`)**
  - **Description**: Vector-style arcade flight game down an endless 3D trench.
  - **Gameplay**:
    - Dodge moving pillars, blast doors, and laser barriers.
    - Score counter and speed progression.

- [ ] **Raycaster 3D Dungeon Crawler (`src/maze_3d.c`)**
  - **Description**: Classic DDA raycasting engine (Wolfenstein 3D style) rendered in ASCII.
  - **Gameplay**:
    - Textured walls, doors, overhead mini-map HUD, and item collection.
