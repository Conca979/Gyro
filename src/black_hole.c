#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <sys/ioctl.h>
#include <termios.h>
#include <fcntl.h>
#endif

// Vector Math
typedef struct { float x, y, z; } vec3;

static inline vec3 add(vec3 a, vec3 b) { return (vec3){a.x + b.x, a.y + b.y, a.z + b.z}; }
static inline vec3 sub(vec3 a, vec3 b) { return (vec3){a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline vec3 scale(vec3 v, float s) { return (vec3){v.x * s, v.y * s, v.z * s}; }
static inline float dot(vec3 a, vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline vec3 cross(vec3 a, vec3 b) {
  return (vec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
static inline float length(vec3 v) { return sqrtf(dot(v, v)); }
static inline vec3 normalize(vec3 v) {
  float len = length(v);
  return len > 0 ? scale(v, 1.0f / len) : (vec3){0,0,0};
}

// Raymarching State
typedef struct {
  vec3 p;
  vec3 v;
} RayState;

// Geodesic acceleration: a = -1.5 * Rs * |p x v|^2 / r^5 * p
static inline vec3 calc_accel(vec3 p, vec3 v, float Rs) {
  float r2 = dot(p, p);
  if (r2 < 0.0001f) return (vec3){0,0,0};
  vec3 h = cross(p, v);
  float h2 = dot(h, h);
  float r5 = r2 * r2 * sqrtf(r2);
  float coef = -1.5f * Rs * h2 / r5;
  return scale(p, coef);
}

// RK4 Integrator Step
static inline void rk4_step(RayState* state, float dt, float Rs) {
  vec3 p = state->p;
  vec3 v = state->v;

  vec3 k1_v = calc_accel(p, v, Rs);
  vec3 k1_p = v;

  vec3 p2 = add(p, scale(k1_p, dt * 0.5f));
  vec3 v2 = add(v, scale(k1_v, dt * 0.5f));
  vec3 k2_v = calc_accel(p2, v2, Rs);
  vec3 k2_p = v2;

  vec3 p3 = add(p, scale(k2_p, dt * 0.5f));
  vec3 v3 = add(v, scale(k2_v, dt * 0.5f));
  vec3 k3_v = calc_accel(p3, v3, Rs);
  vec3 k3_p = v3;

  vec3 p4 = add(p, scale(k3_p, dt));
  vec3 v4 = add(v, scale(k3_v, dt));
  vec3 k4_v = calc_accel(p4, v4, Rs);
  vec3 k4_p = v4;

  vec3 sum_p = add(add(k1_p, scale(k2_p, 2.0f)), add(scale(k3_p, 2.0f), k4_p));
  vec3 sum_v = add(add(k1_v, scale(k2_v, 2.0f)), add(scale(k3_v, 2.0f), k4_v));

  state->p = add(p, scale(sum_p, dt / 6.0f));
  state->v = add(v, scale(sum_v, dt / 6.0f));
}

// Skybox Background Calculation
static inline vec3 calc_background(vec3 dir, float time) {
  vec3 col = {0.0f, 0.0f, 0.0f};
  
  // Slowly rotate the skybox to simulate movement
  float t_rot = time * 0.1f;
  float cos_t = cosf(t_rot);
  float sin_t = sinf(t_rot);
  vec3 rot_dir = {
    dir.x * cos_t - dir.z * sin_t,
    dir.y,
    dir.x * sin_t + dir.z * cos_t
  };
  dir = rot_dir;
  
  // Galactic band (Milky way style)
  float band = expf(-powf(fabsf(dir.y) * 4.0f, 2.0f));
  float noise = (sinf(10.0f * dir.x) * cosf(10.0f * dir.z) * 0.5f + 0.5f);
  col.x += band * noise * 20.0f; // R
  col.y += band * noise * 30.0f; // G
  col.z += band * 50.0f;         // B
  
  // Celestial Grid
  float phi = atan2f(dir.z, dir.x);
  float theta = asinf(dir.y);
  float grid_size = 0.2f;
  float line_width = 0.01f;
  if (fabsf(fmodf(phi + grid_size*0.5f, grid_size) - grid_size*0.5f) < line_width || 
    fabsf(fmodf(theta + grid_size*0.5f, grid_size) - grid_size*0.5f) < line_width) {
    col.x += 20.0f;
    col.y += 20.0f;
    col.z += 40.0f;
  }
  
  // Clamp
  if (col.x > 255.0f) col.x = 255.0f;
  if (col.y > 255.0f) col.y = 255.0f;
  if (col.z > 255.0f) col.z = 255.0f;
  
  return col;
}

// Map Intensity to TrueColor and ASCII
static void apply_color(float intensity, vec3 bg_color, char* out_buf) {
  int bg_r = (int)bg_color.x;
  int bg_g = (int)bg_color.y;
  int bg_b = (int)bg_color.z;

  if (intensity < 0.01f) {
    sprintf(out_buf, "\x1b[48;2;%d;%d;%dm \x1b[0m", bg_r, bg_g, bg_b);
    return;
  }
  
  // ASCII ramp
  const char* ramp = " .,-~:;=!*#$@";
  int num_chars = strlen(ramp);
  
  // Scale intensity for visual effect
  float visual_i = intensity * 1.5f;
  int char_idx = (int)(visual_i * (num_chars - 1) / 3.0f);
  if (char_idx < 0) char_idx = 0;
  if (char_idx >= num_chars) char_idx = num_chars - 1;
  char c = ramp[char_idx];

  // Color gradient mapping
  int r = 0, g = 0, b = 0;
  if (visual_i < 0.5f) {
    r = (int)(visual_i * 2.0f * 255.0f); // Black to Red
  } else if (visual_i < 1.5f) {
    r = 255;
    g = (int)((visual_i - 0.5f) * 255.0f); // Red to Yellow
  } else if (visual_i < 2.5f) {
    r = 255;
    g = 255;
    b = (int)((visual_i - 1.5f) * 255.0f); // Yellow to White
  } else {
    r = 255 - (int)((visual_i - 2.5f) * 255.0f); // White to Cyan/Blue
    if (r < 0) r = 0;
    g = 255;
    b = 255;
  }
  
  if (r > 255) r = 255;
  if (g > 255) g = 255;
  if (b > 255) b = 255;

  sprintf(out_buf, "\x1b[38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm%c\x1b[0m", r, g, b, bg_r, bg_g, bg_b, c);
}

// Read keyboard without blocking
static int kbhit_local() {
#ifdef _WIN32
  return _kbhit();
#else
  struct termios oldt, newt;
  int ch;
  int oldf;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
  if (ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }
  return 0;
#endif
}

static int getch_local() {
#ifdef _WIN32
  return _getch();
#else
  struct termios oldt, newt;
  int ch;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return ch;
#endif
}

void get_terminal_size(int *width, int *height) {
#ifdef _WIN32
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
    *width = 80;
    *height = 24;
  } else {
    *width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
  }
#else
  struct winsize w;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col == 0 || w.ws_row == 0) {
    *width = 80;
    *height = 24;
  } else {
    *width = w.ws_col;
    *height = w.ws_row;
  }
#endif
  if (*width <= 0) *width = 80;
  if (*height <= 0) *height = 24;

  // Scale constraints
  *width = (int)(*width * 0.95f);
  *height = (int)(*height * 0.95f);
}

int main() {
#ifdef _WIN32
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD dwMode = 0;
  GetConsoleMode(hOut, &dwMode);
  SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif
 
  setvbuf(stdout, NULL, _IONBF, 0);
  printf("\x1b[2J"); // Clear screen
  
  int width = 0, height = 0;
  char *renderBuffer = NULL;
  char **rowBuffers = NULL;

  float Rs = 1.0f; // Schwarzschild radius
  float R_inner = 2.6f * Rs;
  float R_outer = 8.0f * Rs;
  
  float camera_dist = 12.0f;
  float camera_yaw = 0.0f;
  float camera_pitch = 0.2f;

  float t = 0;

  
  while (1) {
    if (kbhit_local()) {
      int key = getch_local();
      if (key == 27) break; // ESC
      if (key == 'w' || key == 'W') camera_pitch -= 0.1f;
      if (key == 's' || key == 'S') camera_pitch += 0.1f;
      if (key == 'a' || key == 'A') camera_yaw -= 0.1f;
      if (key == 'd' || key == 'D') camera_yaw += 0.1f;
      if (key == '=') camera_dist -= 0.5f;
      if (key == '-') camera_dist += 0.5f;
      // Arrow keys (ANSI escape sequence handling for Linux/Mac and Windows)
      if (key == 224 || key == 91) { // Windows extended or Unix CSI
        int arr = getch_local();
        if (arr == 91) arr = getch_local(); // Mac/Linux double sequence
        if (arr == 'A' || arr == 72) camera_pitch -= 0.1f; // Up
        if (arr == 'B' || arr == 80) camera_pitch += 0.1f; // Down
        if (arr == 'C' || arr == 77) camera_yaw += 0.1f;   // Right
        if (arr == 'D' || arr == 75) camera_yaw -= 0.1f;   // Left
      }
    }
    
    if (camera_pitch > 1.5f) camera_pitch = 1.5f;
    if (camera_pitch < -1.5f) camera_pitch = -1.5f;
    if (camera_dist < 3.0f) camera_dist = 3.0f;

    int new_width, new_height;
    get_terminal_size(&new_width, &new_height);
    
    if (new_width != width || new_height != height) {
      int old_height = height;
      width = new_width;
      height = new_height;
      renderBuffer = realloc(renderBuffer, height * (width * 64 + 2)); 
      if (rowBuffers) {
        for(int i = 0; i < old_height; i++) if (rowBuffers[i]) free(rowBuffers[i]);
        free(rowBuffers);
      }
      rowBuffers = malloc(height * sizeof(char*));
      for(int i = 0; i < height; i++) rowBuffers[i] = malloc(width * 64 + 2);
    }

    vec3 cam_pos = {
      camera_dist * cosf(camera_pitch) * sinf(camera_yaw),
      camera_dist * sinf(camera_pitch),
      camera_dist * cosf(camera_pitch) * cosf(camera_yaw)
    };
    
    vec3 forward = normalize(sub((vec3){0,0,0}, cam_pos));
    vec3 world_up = {0, 1, 0};
    vec3 right = normalize(cross(forward, world_up));
    vec3 up = cross(right, forward);

    float fov = 1.0f;
    float aspect = (float)width / (float)(height * 2.0f); // Terminal characters are ~2x as tall as they are wide

    // Parallelized raymarching loop!
    #pragma omp parallel for schedule(dynamic)
    for (int y = 0; y < height; y++) {
      char* r_ptr = rowBuffers[y];
      
      for (int x = 0; x < width; x++) {
        float ndc_x = (2.0f * (x + 0.5f) / (float)width - 1.0f) * aspect * fov;
        float ndc_y = (1.0f - 2.0f * (y + 0.5f) / (float)height) * fov;
        
        vec3 ray_dir = normalize(add(add(forward, scale(right, ndc_x)), scale(up, ndc_y)));
        
        RayState state = {cam_pos, ray_dir};
        float intensity = 0.0f;
        float ray_energy = 1.0f;
        vec3 bg_color = {0, 0, 0};
        
        int max_steps = 800;
        float escape_r2 = 400.0f;
        
        for (int step = 0; step < max_steps; step++) {
          float r = length(state.p);
          if (r < Rs * 1.05f) {
            break; // Absorbed by event horizon
          }
          if (r * r > escape_r2) {
            bg_color = calc_background(normalize(state.v), t);
            bg_color = scale(bg_color, ray_energy);
            break; // Escaped
          }

          // Adaptive step size based on distance to event horizon
          float dist_to_horizon = r - Rs;
          
          float dt = 0.05f * dist_to_horizon;
          if (dt < 0.02f) dt = 0.02f; // minimum step
          if (dt > 0.5f) dt = 0.5f;   // maximum step
          
          vec3 prev_p = state.p;
          rk4_step(&state, dt, Rs);
          
          // Check disk intersection (crossing y=0)
          if (prev_p.y * state.p.y <= 0.0f) {
            float t_intersect = -prev_p.y / (state.p.y - prev_p.y);
            vec3 hit_p = add(prev_p, scale(sub(state.p, prev_p), t_intersect));
            float hit_r = sqrtf(hit_p.x * hit_p.x + hit_p.z * hit_p.z);
            
            if (hit_r > R_inner && hit_r < R_outer) {
              // Disk orbital velocity (Newtonian approx for stability, accurate visually)
              float v_orb = 1.0f / sqrtf(2.0f * (hit_r - Rs)); 
              vec3 V_fluid = {-hit_p.z / hit_r * v_orb, 0.0f, hit_p.x / hit_r * v_orb};
              
              // Doppler shift
              float dot_v = dot(normalize(state.v), V_fluid);
              float doppler = sqrtf(1.0f - v_orb * v_orb) / (1.0f - dot_v);
              
              // Analytic disk texture (trigonometric banding)
              float phi = atan2f(hit_p.z, hit_p.x);
              float tex = (sinf(10.0f * hit_r) * 0.5f + 0.5f) * (cosf(4.0f * phi - t * 2.0f) * 0.5f + 0.5f);
              float base_glow = 1.5f / (hit_r - Rs);
              
              float add_intensity = (base_glow + tex * 0.5f) * doppler * doppler * doppler;
              intensity += add_intensity * ray_energy;
              ray_energy *= 0.3f; // Semi-transparent disk
              
              if (ray_energy < 0.05f) break; // Opaque enough
            }
          }
        }
        
        char cell_buf[64];
        apply_color(intensity, bg_color, cell_buf);
        strcpy(r_ptr, cell_buf);
        r_ptr += strlen(cell_buf);
      }
      strcpy(r_ptr, "\n");
    }
    
    // Concatenate all row buffers sequentially to the main renderBuffer
    renderBuffer[0] = '\0';
    char* main_ptr = renderBuffer;
    for (int y = 0; y < height; y++) {
      strcpy(main_ptr, rowBuffers[y]);
      main_ptr += strlen(rowBuffers[y]);
    }
    
    // Reset cursor to top-left
    size_t total_bytes = main_ptr - renderBuffer;
    printf("\x1b[H");
    fwrite(renderBuffer, 1, total_bytes, stdout);
    
    static int first_frame = 1;
    if (first_frame) {
      FILE *f = fopen("frame.txt", "wb");
      if (f) {
        fwrite(renderBuffer, 1, total_bytes, f);
        fclose(f);
      }
      first_frame = 0;
    }
    
    t += 0.05f;
#ifdef _WIN32
    Sleep(16.67); // Lock to ~60 FPS for stability under heavy raymarching load
#else
    usleep(33000);
#endif
  }
  
  // Free resources
  if (renderBuffer) free(renderBuffer);
  if (rowBuffers) {
    for(int i=0; i<height; i++) if (rowBuffers[i]) free(rowBuffers[i]);
    free(rowBuffers);
  }

  return 0;
}
