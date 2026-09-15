#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <windows.h>
#include <time.h>

#define MAX_STEPS 100
#define SURF_DIST 0.01f
#define MAX_DIST 100.0f
#define MAP_SIZE 16

// Vector Math
typedef struct { float x, y, z; } vec3;
vec3 add(vec3 a, vec3 b) { return (vec3){a.x + b.x, a.y + b.y, a.z + b.z}; }
vec3 sub(vec3 a, vec3 b) { return (vec3){a.x - b.x, a.y - b.y, a.z - b.z}; }
vec3 scale(vec3 a, float s) { return (vec3){a.x * s, a.y * s, a.z * s}; }
float dot(vec3 a, vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
vec3 cross(vec3 a, vec3 b) { return (vec3){a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x}; }
float length(vec3 a) { return sqrtf(dot(a, a)); }
vec3 normalize(vec3 a) { float l = length(a); return l > 0 ? scale(a, 1.0f/l) : (vec3){0,0,0}; }

vec3 rotate_x(vec3 p, float a) {
  float s = sinf(a), c = cosf(a);
  return (vec3){p.x, p.y * c - p.z * s, p.y * s + p.z * c};
}
vec3 rotate_y(vec3 p, float a) {
  float s = sinf(a), c = cosf(a);
  return (vec3){p.x * c + p.z * s, p.y, -p.x * s + p.z * c};
}
vec3 rotate_z(vec3 p, float a) {
  float s = sinf(a), c = cosf(a);
  return (vec3){p.x * c - p.y * s, p.x * s + p.y * c, p.z};
}

float smin(float a, float b, float k) {
  float h = fmaxf(k - fabsf(a - b), 0.0f) / k;
  return fminf(a, b) - h * h * k * (1.0f / 4.0f);
}

// SDF Primitives
float sdSphere(vec3 p, float s) { return length(p) - s; }
float sdEllipsoid(vec3 p, vec3 r) {
  float k0 = length((vec3){p.x/r.x, p.y/r.y, p.z/r.z});
  float k1 = length((vec3){p.x/(r.x*r.x), p.y/(r.y*r.y), p.z/(r.z*r.z)});
  return k0 * (k0 - 1.0f) / k1;
}
float sdCapsule(vec3 p, vec3 a, vec3 b, float r) {
  vec3 pa = sub(p, a), ba = sub(b, a);
  float h = fmaxf(fminf(dot(pa, ba) / dot(ba, ba), 1.0f), 0.0f);
  return length(sub(pa, scale(ba, h))) - r;
}

// Block-Font Rasterizer
int get_font_bits(char c) {
  if (c >= 'a' && c <= 'z') c -= 32; 
  switch (c) {
    case 'A': return 11245; case 'B': return 27630; case 'C': return 14627;
    case 'D': return 27502; case 'E': return 31143; case 'F': return 31140;
    case 'G': return 14699; case 'H': return 23533; case 'I': return 29847;
    case 'J': return 4714;  case 'K': return 23861; case 'L': return 18727;
    case 'M': return 24429; case 'N': return 31597; case 'O': return 11114;
    case 'P': return 27556; case 'Q': return 11097; case 'R': return 27565;
    case 'S': return 14478; case 'T': return 29842; case 'U': return 23403;
    case 'V': return 23402; case 'W': return 23421; case 'X': return 23213;
    case 'Y': return 23186; case 'Z': return 29351; case '0': return 11114;
    case '1': return 11415; case '2': return 29671; case '3': return 29391;
    case '4': return 23497; case '5': return 31183; case '6': return 14827;
    case '7': return 29330; case '8': return 15083; case '9': return 15051;
    case '[': return 26918; case ']': return 12875; case '/': return 4772;
    case '+': return 1488;  case '-': return 448;   case '=': return 3640;
    case ':': return 1040;  case '|': return 9362;  case ' ': return 0;
    case '\'': return 9216; default: return 0;
  }
}

void draw_scaled_text(char** buffer, int** color_buffer, int start_x, int start_y, const char* text, int scale, int max_w, int max_h, int color) {
  int cur_x = start_x;
  for (int i = 0; text[i] != '\0'; i++) {
    int bits = get_font_bits(text[i]);
    for (int y = 0; y < 5; y++) {
      for (int x = 0; x < 3; x++) {
        int bit_idx = 14 - (y * 3 + x);
        if ((bits >> bit_idx) & 1) {
          for (int sy = 0; sy < scale; sy++) {
            for (int sx = 0; sx < scale; sx++) {
              int py = start_y + (y * scale) + sy;
              int px = cur_x + (x * scale) + sx;
              if (py >= 0 && py < max_h && px >= 0 && px < max_w) {
                buffer[py][px] = '#'; 
                if (color_buffer) color_buffer[py][px] = color;
              }
            }
          }
        }
      }
    }
    cur_x += 4 * scale; // 3 width + 1 spacing
  }
}

// Global state
float time_sec = 0;

// World Map removed - transitioning to Outdoor Open World

// Entity Types
typedef enum {
  TYPE_DUCK,
  TYPE_BIRD,
  TYPE_PLANE,
  TYPE_PIG,
  TYPE_DEER,
  TYPE_COUNT
} EntityType;

int get_score(EntityType t) {
  if (t == TYPE_PIG) return 50;
  if (t == TYPE_DUCK) return 100;
  if (t == TYPE_DEER) return 150;
  if (t == TYPE_BIRD) return 200;
  if (t == TYPE_PLANE) return 500;
  return 100;
}

// Entity Map
float map_entity(vec3 p, EntityType type, float flap) {
  float d = 1000.0f;
  if (type == TYPE_DUCK || type == TYPE_BIRD) {
    float body_rad = (type == TYPE_BIRD) ? 1.0f : 1.5f;
    float body = sdEllipsoid(p, (vec3){body_rad, body_rad*0.5f, body_rad*0.6f});
    vec3 head_pos = {body_rad*0.8f, body_rad*0.6f, 0.0f};
    float neck = sdCapsule(p, (vec3){body_rad*0.3f, body_rad*0.1f, 0.0f}, head_pos, body_rad*0.25f);
    float head = sdSphere(sub(p, head_pos), body_rad*0.3f);
    vec3 beak_pos = {body_rad*1.1f, body_rad*0.6f, 0.0f};
    float beak = sdEllipsoid(sub(p, beak_pos), (vec3){body_rad*0.25f, body_rad*0.1f, body_rad*0.15f});
    
    d = smin(body, neck, 0.3f);
    d = smin(d, head, 0.2f);
    d = fminf(d, beak);
    
    vec3 w_pos = {0.0f, body_rad*0.1f, body_rad*0.6f};
    vec3 p_rw = sub(p, w_pos);
    p_rw = rotate_x(p_rw, flap);
    p_rw = sub(p_rw, (vec3){0.0f, 0.0f, body_rad*0.8f}); 
    float r_wing = sdEllipsoid(p_rw, (vec3){body_rad*0.6f, 0.05f, body_rad*0.8f});
    
    vec3 lw_pos = {0.0f, body_rad*0.1f, -body_rad*0.6f};
    vec3 p_lw = sub(p, lw_pos);
    p_lw = rotate_x(p_lw, -flap);
    p_lw = sub(p_lw, (vec3){0.0f, 0.0f, -body_rad*0.8f});
    float l_wing = sdEllipsoid(p_lw, (vec3){body_rad*0.6f, 0.05f, body_rad*0.8f});
    
    d = smin(d, r_wing, 0.2f);
    d = smin(d, l_wing, 0.2f);
  } else if (type == TYPE_PLANE) {
    float body = sdEllipsoid(p, (vec3){1.5f, 0.1f, 0.1f}); 
    float wings = sdEllipsoid(sub(p, (vec3){-0.5f, 0.0f, 0.0f}), (vec3){1.0f, 0.05f, 1.2f}); 
    d = smin(body, wings, 0.2f);
  } else if (type == TYPE_PIG) {
    float body = sdEllipsoid(p, (vec3){1.5f, 1.0f, 1.2f});
    float snout = sdEllipsoid(sub(p, (vec3){1.5f, 0.0f, 0.0f}), (vec3){0.4f, 0.3f, 0.4f});
    d = smin(body, snout, 0.3f);
    float l1 = sdCapsule(p, (vec3){1.0f, -0.5f, 0.8f}, (vec3){1.0f, -1.5f, 0.8f}, 0.2f);
    float l2 = sdCapsule(p, (vec3){1.0f, -0.5f, -0.8f}, (vec3){1.0f, -1.5f, -0.8f}, 0.2f);
    float l3 = sdCapsule(p, (vec3){-1.0f, -0.5f, 0.8f}, (vec3){-1.0f, -1.5f, 0.8f}, 0.2f);
    float l4 = sdCapsule(p, (vec3){-1.0f, -0.5f, -0.8f}, (vec3){-1.0f, -1.5f, -0.8f}, 0.2f);
    d = smin(d, l1, 0.2f); d = smin(d, l2, 0.2f); d = smin(d, l3, 0.2f); d = smin(d, l4, 0.2f);
  } else if (type == TYPE_DEER) {
    float body = sdCapsule(p, (vec3){-1.0f, 1.0f, 0.0f}, (vec3){1.0f, 1.0f, 0.0f}, 0.6f);
    float neck = sdCapsule(p, (vec3){1.0f, 1.0f, 0.0f}, (vec3){1.5f, 2.5f, 0.0f}, 0.3f);
    float head = sdEllipsoid(sub(p, (vec3){1.7f, 2.7f, 0.0f}), (vec3){0.4f, 0.3f, 0.3f});
    d = smin(body, neck, 0.3f);
    d = smin(d, head, 0.2f);
    float l1 = sdCapsule(p, (vec3){0.8f, 1.0f, 0.4f}, (vec3){0.8f, -1.0f, 0.4f}, 0.15f);
    float l2 = sdCapsule(p, (vec3){0.8f, 1.0f, -0.4f}, (vec3){0.8f, -1.0f, -0.4f}, 0.15f);
    float l3 = sdCapsule(p, (vec3){-0.8f, 1.0f, 0.4f}, (vec3){-0.8f, -1.0f, 0.4f}, 0.15f);
    float l4 = sdCapsule(p, (vec3){-0.8f, 1.0f, -0.4f}, (vec3){-0.8f, -1.0f, -0.4f}, 0.15f);
    d = smin(d, l1, 0.2f); d = smin(d, l2, 0.2f); d = smin(d, l3, 0.2f); d = smin(d, l4, 0.2f);
    float a1 = sdCapsule(p, (vec3){1.5f, 2.8f, 0.2f}, (vec3){1.0f, 3.5f, 0.5f}, 0.08f);
    float a2 = sdCapsule(p, (vec3){1.5f, 2.8f, -0.2f}, (vec3){1.0f, 3.5f, -0.5f}, 0.08f);
    d = smin(d, a1, 0.1f); d = smin(d, a2, 0.1f);
  }
  return d;
}

typedef struct {
  vec3 pos;
  vec3 vel;
  bool is_alive;
  bool is_falling;
  float yaw;
  float state_timer;
  EntityType type;
} Entity3D;

Entity3D entities[10];
int num_entities = 5;

float map_scene(vec3 p, int* out_color) {
  // Infinite Ground Plane
  float min_d = p.y + 2.0f;
  if (out_color) *out_color = 32; // Green
  
  // Infinite Forest (Domain Repetition)
  vec3 q = p;
  q.x = fmodf(p.x + 10000.0f, 20.0f) - 10.0f;
  q.z = fmodf(p.z + 10000.0f, 20.0f) - 10.0f;
  
  // Tree Trunk
  float trunk = fmaxf(sqrtf(q.x*q.x + q.z*q.z) - 0.4f, fabsf(q.y - 1.0f) - 3.0f);
  // Tree Leaves
  float leaves = sdSphere(sub(q, (vec3){0.0f, 4.0f, 0.0f}), 2.5f);
  
  float tree = smin(trunk, leaves, 0.5f);
  
  if (tree < min_d) {
    min_d = tree;
    if (out_color) *out_color = 33; // Yellow/Brown
  }
  
  for (int i = 0; i < num_entities; i++) {
    if (entities[i].is_alive || entities[i].is_falling) {
      vec3 lp = sub(p, entities[i].pos);
      
      // Bounding Sphere Check (Radius 2.5f)
      float bound_dist = length(lp) - 2.5f; 
      
      if (bound_dist > 0.5f) {
        if (bound_dist < min_d) {
          min_d = bound_dist;
          if (out_color) *out_color = 0; // Empty/bound
        }
      } else {
        lp = rotate_y(lp, entities[i].yaw);
        if (entities[i].is_falling) lp = rotate_z(lp, 1.57f);
        
        float d = map_entity(scale(lp, 2.0f), entities[i].type, sinf(time_sec * 15.0f) * 0.8f) * 0.5f;
        if (d < min_d) {
          min_d = d;
          if (out_color) {
            if (entities[i].type == TYPE_DUCK) *out_color = 93;
            else if (entities[i].type == TYPE_PIG) *out_color = 95;
            else if (entities[i].type == TYPE_DEER) *out_color = 33;
            else if (entities[i].type == TYPE_BIRD) *out_color = 96;
            else if (entities[i].type == TYPE_PLANE) *out_color = 97;
          }
        }
      }
    }
  }
  return min_d;
}

float ray_march(vec3 ro, vec3 rd, float max_dist) {
  float dO = 0;
  for (int i = 0; i < MAX_STEPS; i++) {
    vec3 p = add(ro, scale(rd, dO));
    float dS = map_scene(p, NULL);
    dO += dS;
    if (dO > max_dist || dS < SURF_DIST) break;
  }
  return dO;
}

vec3 calc_normal(vec3 p) {
  float d = map_scene(p, NULL);
  vec3 n = {
    d - map_scene((vec3){p.x - 0.01f, p.y, p.z}, NULL),
    d - map_scene((vec3){p.x, p.y - 0.01f, p.z}, NULL),
    d - map_scene((vec3){p.x, p.y, p.z - 0.01f}, NULL)
  };
  return normalize(n);
}

void get_terminal_size(int *width, int *height) {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
    *width = 80;
    *height = 24;
  } else {
    *width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
  }
  *width = (int)(*width * 0.95f);
  *height = (int)(*height * 0.95f);
}

int main(int argc, char *argv[]) {


  srand((unsigned int)time(NULL));
  if (argc > 1 && (strcmp(argv[1], "--mode=B") == 0 || strcmp(argv[1], "--mode=b") == 0)) {
    num_entities = 10;
  }

  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD outMode = 0;
  GetConsoleMode(hOut, &outMode);
  SetConsoleMode(hOut, outMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);

  setvbuf(stdout, NULL, _IONBF, 0);
  printf("\x1b[2J\x1b[?25l"); 
  fflush(stdout);
  
  for(int i=0; i<num_entities; i++) {
    entities[i].type = (EntityType)(rand() % TYPE_COUNT);
    entities[i].is_alive = true;
    entities[i].is_falling = false;
    entities[i].state_timer = 1.0f + (rand()%20)*0.1f;
    
    if (entities[i].type == TYPE_PIG || entities[i].type == TYPE_DEER) {
      entities[i].pos = (vec3){2.0f + (rand()%10), -1.0f, 8.0f + (rand()%6)};
      entities[i].vel = (vec3){0, 0, 0};
      entities[i].yaw = 0.0f;
    } else {
      entities[i].pos = (vec3){2.0f + (rand()%10), 3.0f + (rand()%6), 8.0f + (rand()%6)};
      entities[i].vel = (vec3){(rand()%4)-2.0f, (rand()%2)-1.0f, (rand()%4)-2.0f};
      if (entities[i].vel.x == 0 && entities[i].vel.z == 0) entities[i].vel.x = 1.0f;
      entities[i].yaw = atan2f(entities[i].vel.z, entities[i].vel.x);
    }
  }
  
  int ammo = 8;
  int score = 0;
  int kill_count = 0;
  
  typedef enum { STATE_PLAYING, STATE_GAMEOVER } GameState;
  GameState game_state = STATE_PLAYING;
  float game_timer = 60.0f;
  float reload_timer = 0.0f;
  
  // Camera state
  vec3 ro = {2.0f, 0.0f, 2.0f};
  float cam_yaw = 0.7f;
  float cam_pitch = 0.0f;
  
  LARGE_INTEGER frequency, last_time, current_time;
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&last_time);
  
  bool running = true;
  bool mouse_locked = true;
  bool was_esc_down = false;
  bool was_lbutton_down = false;
  bool was_r_down = false;
  
  char* renderBuffer = NULL;
  char** rowBuffers = NULL;
  int** colorBuffers = NULL;
  float* z_buffer = NULL;
  int last_w = 0, last_h = 0;
  
  const char* shading = " .,-~:;=!*#$@";
  
  
  while (running) {
    int w, h;
    get_terminal_size(&w, &h);
    
    if (w != last_w || h != last_h) {
      renderBuffer = realloc(renderBuffer, h * (w + 2) * 20 + 2048); 
      z_buffer = realloc(z_buffer, w * sizeof(float));
      if (rowBuffers) {
        for(int i=0; i<last_h; i++) {
          free(rowBuffers[i]);
          free(colorBuffers[i]);
        }
        free(rowBuffers);
        free(colorBuffers);
      }
      rowBuffers = malloc(h * sizeof(char*));
      colorBuffers = malloc(h * sizeof(int*));
      for(int i=0; i<h; i++) {
        rowBuffers[i] = malloc(w + 2);
        colorBuffers[i] = malloc((w + 2) * sizeof(int));
      }
      last_w = w; last_h = h;
    }
    
    QueryPerformanceCounter(&current_time);
    float dt = (float)(current_time.QuadPart - last_time.QuadPart) / frequency.QuadPart;
    last_time = current_time;
    if (dt > 0.1f) dt = 0.1f;
    time_sec += dt;
    
    bool just_fired = false;
    
    // Safety & Input
    bool is_esc_down = (GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0;
    if (is_esc_down && !was_esc_down) {
      mouse_locked = !mouse_locked;
      ShowCursor(!mouse_locked);
    }
    was_esc_down = is_esc_down;
    
    if (GetAsyncKeyState('Q') & 0x8000) running = false;
    
    bool is_e_down = (GetAsyncKeyState('E') & 0x8000) != 0;
    if (is_e_down && !was_r_down && reload_timer <= 0.0f && ammo < 8) {
      reload_timer = 1.5f;
    }
    was_r_down = is_e_down;
    
    if (reload_timer > 0.0f) {
      reload_timer -= dt;
      if (reload_timer <= 0.0f) ammo = 8;
    }
    
    if (game_state == STATE_PLAYING && !is_esc_down) {
      game_timer -= dt;
      if (game_timer <= 0.0f) {
        game_timer = 0.0f;
        game_state = STATE_GAMEOVER;
      }
    }
    
    if (game_state == STATE_GAMEOVER) {
      if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
        game_state = STATE_PLAYING;
        game_timer = 60.0f;
        score = 0;
        kill_count = 0;
        ammo = 8;
        reload_timer = 0.0f;
        for(int i=0; i<num_entities; i++) entities[i].is_falling = true; // force respawn
      }
    }
    
    bool is_lbutton = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    
    if (!mouse_locked && is_lbutton && !was_lbutton_down) {
      mouse_locked = true;
      ShowCursor(FALSE);
      
      // Seed the mouse to the center immediately to avoid a huge initial jump
      SetCursorPos(GetSystemMetrics(SM_CXSCREEN) / 2, GetSystemMetrics(SM_CYSCREEN) / 2);
    }
    
    if (mouse_locked) {
      // FIX: Do not use GetConsoleWindow() as it fails in Windows Terminal.
      // Simply lock the mouse to the absolute center of the primary monitor.
      int center_x = GetSystemMetrics(SM_CXSCREEN) / 2;
      int center_y = GetSystemMetrics(SM_CYSCREEN) / 2;
      
      POINT pt;
      GetCursorPos(&pt);
      
      float dx = (pt.x - center_x) * 0.003f;
      float dy = (pt.y - center_y) * 0.003f;
      
      cam_yaw += dx;
      cam_pitch -= dy; // Invert Y
      if (cam_pitch > 1.5f) cam_pitch = 1.5f;
      if (cam_pitch < -1.5f) cam_pitch = -1.5f;
      
      SetCursorPos(center_x, center_y);
      

      
      // Shooting
      if (is_lbutton && !was_lbutton_down && ammo > 0 && reload_timer <= 0.0f && game_state == STATE_PLAYING) {
        ammo--;
        just_fired = true;
        vec3 forward = {cosf(cam_pitch)*cosf(cam_yaw), sinf(cam_pitch), cosf(cam_pitch)*sinf(cam_yaw)};
        float d = ray_march(ro, forward, MAX_DIST);
        if (d < MAX_DIST) {
          vec3 hit_p = add(ro, scale(forward, d));
          for(int d_idx=0; d_idx<num_entities; d_idx++) {
            if (entities[d_idx].is_alive) {
              if (length(sub(hit_p, entities[d_idx].pos)) < 2.0f) { 
                entities[d_idx].is_alive = false;
                entities[d_idx].is_falling = true;
                score += get_score(entities[d_idx].type);
                kill_count++;
              }
            }
          }
        }
      }
    }
    was_lbutton_down = is_lbutton;
    
    // WASD Movement
    float move_speed = 5.0f * dt;
    vec3 forward_flat = {cosf(cam_yaw), 0.0f, sinf(cam_yaw)};
    vec3 right_flat = {cosf(cam_yaw + 1.5708f), 0.0f, sinf(cam_yaw + 1.5708f)};
    
    bool is_moving = false;
    if (mouse_locked) {
      if (GetAsyncKeyState('W') & 0x8000) {
        ro.x += forward_flat.x * move_speed;
        ro.z += forward_flat.z * move_speed;
        is_moving = true;
      }
      if (GetAsyncKeyState('S') & 0x8000) {
        ro.x -= forward_flat.x * move_speed;
        ro.z -= forward_flat.z * move_speed;
        is_moving = true;
      }
      if (GetAsyncKeyState('D') & 0x8000) {
        ro.x += right_flat.x * move_speed;
        ro.z += right_flat.z * move_speed;
        is_moving = true;
      }
      if (GetAsyncKeyState('A') & 0x8000) {
        ro.x -= right_flat.x * move_speed;
        ro.z -= right_flat.z * move_speed;
        is_moving = true;
      }
    }
    
    // Player Wall collision removed for Open World!
    
    // Entity Physics
    if (game_state == STATE_PLAYING) {
      for (int i=0; i<num_entities; i++) {
        if (entities[i].is_alive) {
          
          if (entities[i].type == TYPE_PIG || entities[i].type == TYPE_DEER) {
            // Ground AI
            float dist = length(sub(entities[i].pos, ro));
            if (dist < 15.0f && entities[i].vel.x == 0 && entities[i].vel.z == 0) {
              vec3 away = normalize((vec3){entities[i].pos.x - ro.x, 0.0f, entities[i].pos.z - ro.z});
              float speed = (entities[i].type == TYPE_DEER) ? 6.0f : 2.5f;
              entities[i].vel = (vec3){away.x * speed, 0.0f, away.z * speed};
              entities[i].yaw = atan2f(entities[i].vel.z, entities[i].vel.x);
            } else if (dist >= 25.0f) {
              entities[i].vel = (vec3){0,0,0};
            }
            entities[i].pos.x += entities[i].vel.x * dt;
            entities[i].pos.z += entities[i].vel.z * dt;
            
            // Bounds tethering
            if (entities[i].pos.x > ro.x + 20.0f || entities[i].pos.x < ro.x - 20.0f) {
              entities[i].vel.x *= -1; entities[i].yaw = atan2f(entities[i].vel.z, entities[i].vel.x);
            }
            if (entities[i].pos.z > ro.z + 20.0f || entities[i].pos.z < ro.z - 20.0f) {
              entities[i].vel.z *= -1; entities[i].yaw = atan2f(entities[i].vel.z, entities[i].vel.x);
            }
          } else {
            // Flying AI
            entities[i].state_timer -= dt;
            if (entities[i].state_timer <= 0.0f) {
              float spd = (entities[i].type == TYPE_PLANE) ? 6.0f : (entities[i].type == TYPE_BIRD ? 4.0f : 2.0f);
              entities[i].vel = (vec3){(rand()%6)-3.0f, (rand()%4)-2.0f, (rand()%6)-3.0f};
              entities[i].vel = scale(normalize(entities[i].vel), spd);
              if (entities[i].vel.x == 0 && entities[i].vel.z == 0) entities[i].vel.x = spd;
              entities[i].yaw = atan2f(entities[i].vel.z, entities[i].vel.x);
              entities[i].state_timer = 1.0f + (rand()%20)*0.1f;
            }
            
            entities[i].pos = add(entities[i].pos, scale(entities[i].vel, dt));
            
            if (entities[i].pos.x > ro.x + 20.0f || entities[i].pos.x < ro.x - 20.0f) {
              entities[i].vel.x *= -1; entities[i].yaw = atan2f(entities[i].vel.z, entities[i].vel.x);
            }
            if (entities[i].pos.z > ro.z + 20.0f || entities[i].pos.z < ro.z - 20.0f) {
              entities[i].vel.z *= -1; entities[i].yaw = atan2f(entities[i].vel.z, entities[i].vel.x);
            }
            if (entities[i].pos.y > 10.0f) entities[i].vel.y = -fabsf(entities[i].vel.y);
            if (entities[i].pos.y < 1.0f) entities[i].vel.y = fabsf(entities[i].vel.y);
          }
          
        } else if (entities[i].is_falling) {
          entities[i].pos.y -= 10.0f * dt;
        }
        
        // Respawn logic
        if (entities[i].pos.y < -2.0f) {
          entities[i].type = (EntityType)(rand() % TYPE_COUNT);
          if (entities[i].type == TYPE_PIG || entities[i].type == TYPE_DEER) {
            entities[i].pos = (vec3){ro.x + (rand()%20)-10.0f, -1.0f, ro.z + (rand()%20)-10.0f};
            entities[i].vel = (vec3){0, 0, 0};
            entities[i].yaw = 0.0f;
          } else {
            entities[i].pos = (vec3){ro.x + (rand()%20)-10.0f, 3.0f + (rand()%3), ro.z + (rand()%20)-10.0f};
            float spd = (entities[i].type == TYPE_PLANE) ? 6.0f : (entities[i].type == TYPE_BIRD ? 4.0f : 2.0f);
            entities[i].vel = (vec3){(rand()%4)-2.0f, (rand()%2)-1.0f, (rand()%4)-2.0f};
            entities[i].vel = scale(normalize(entities[i].vel), spd);
            if (entities[i].vel.x == 0 && entities[i].vel.z == 0) entities[i].vel.x = spd;
            entities[i].yaw = atan2f(entities[i].vel.z, entities[i].vel.x);
          }
          entities[i].is_alive = true;
          entities[i].is_falling = false;
          entities[i].state_timer = 2.0f;
        }
      }
    }
    
    if (game_state == STATE_GAMEOVER) {
      for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) rowBuffers[y][x] = ' ';
        rowBuffers[y][w] = '\n'; rowBuffers[y][w+1] = '\0';
      }
      
      int scale = h / 20; 
      if (scale < 1) scale = 1;
      
      int go_width = 9 * 4 * scale;
      int start_x = w/2 - go_width/2;
      int start_y = h/2 - (10 * scale);
      
      draw_scaled_text(rowBuffers, colorBuffers, start_x, start_y, "GAME OVER", scale, w, h, 91); // Bright Red
      
      int sub_scale = scale / 2;
      if (sub_scale < 1) sub_scale = 1;
      
      char msg[100];
      snprintf(msg, sizeof(msg), "FINAL SCORE: %d", score);
      int msg_len = strlen(msg);
      int sub_width = msg_len * 4 * sub_scale;
      draw_scaled_text(rowBuffers, colorBuffers, w/2 - sub_width/2, start_y + (6 * scale) + (2 * sub_scale), msg, sub_scale, w, h, 97); // White
      
      const char* rm = "PRESS SPACE TO PLAY AGAIN";
      int rm_len = strlen(rm);
      int rm_width = rm_len * 4 * sub_scale;
      draw_scaled_text(rowBuffers, colorBuffers, w/2 - rm_width/2, start_y + (6 * scale) + (9 * sub_scale), rm, sub_scale, w, h, 97); // White
    } else {
      // --- 1. SDF Pass (Open World & Actors) ---
      vec3 light_dir = normalize((vec3){1.0f, 1.0f, -0.5f});
      
      int ui_width = (int)(w * 0.25f);
      if (ui_width < 30) ui_width = 30;
      int game_w = w - ui_width;
      
      float aspect = (float)game_w / (float)h;
      float aspect_char = aspect * 0.5f;

      #pragma omp parallel for
      for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
          if (x < game_w) {
            float u = (2.0f * x / (float)game_w) - 1.0f;
            float v = (2.0f * y / (float)h) - 1.0f;
            u *= aspect_char;
            
            vec3 forward = {cosf(cam_pitch)*cosf(cam_yaw), sinf(cam_pitch), cosf(cam_pitch)*sinf(cam_yaw)};
            vec3 right = {cosf(cam_yaw + 1.5708f), 0.0f, sinf(cam_yaw + 1.5708f)};
            vec3 up = normalize(cross(right, forward));
            
            vec3 rd = normalize(add(forward, add(scale(right, u), scale(up, -v))));
            
            float d = ray_march(ro, rd, MAX_DIST);
            
            if (d < MAX_DIST) {
              vec3 p = add(ro, scale(rd, d));
              vec3 n = calc_normal(p);
              
              int hit_color = 0;
              map_scene(p, &hit_color);
              colorBuffers[y][x] = hit_color;
              
              float dif = fmaxf(dot(n, light_dir), 0.0f);
              float intensity = 0.2f + 0.8f * dif;
              if (p.y < -1.9f) {
                 if ((((int)floorf(p.x) + (int)floorf(p.z)) % 2) == 0) intensity *= 0.5f;
              }
              int char_idx = (int)(intensity * 12.0f);
              if (char_idx < 0) char_idx = 0; 
              if (char_idx > 12) char_idx = 12;
              rowBuffers[y][x] = shading[char_idx];
            } else {
              colorBuffers[y][x] = 0;
              if (rd.y > 0.2f) { rowBuffers[y][x] = ' '; colorBuffers[y][x] = 0; }
              else if (rd.y > 0.05f) { rowBuffers[y][x] = '.'; colorBuffers[y][x] = 90; } // Dark Gray clouds
              else { rowBuffers[y][x] = '-'; colorBuffers[y][x] = 90; }
            }
          } else if (x == game_w) {
            rowBuffers[y][x] = '|';
            colorBuffers[y][x] = 97; // White border
          } else {
            rowBuffers[y][x] = ' ';
            colorBuffers[y][x] = 0;
          }
        }
        rowBuffers[y][w] = '\n';
        colorBuffers[y][w] = 0;
        rowBuffers[y][w+1] = '\0';
      }
      
      // Crosshair
      rowBuffers[h/2][game_w/2] = '+';
      colorBuffers[h/2][game_w/2] = 91; // Red crosshair
      
      // FPS Viewmodel
      const char* gun_sprite[] = { "   .==.   ", "   ||||   ", "   ||||   ", "  /||||\\  ", " /||||||\\ ", "/_||||||_\\" };
      int gun_h = 6, gun_w = 10;
      int bob = (int)(sinf(time_sec * 8.0f) * 1.5f * (is_moving ? 1.0f : 0.0f));
      if (just_fired) bob += 2;
      int gun_x = game_w / 2 - gun_w / 2;
      int gun_y = h - gun_h + bob;
      
      if (just_fired && ammo >= 0) {
        if (gun_y - 2 >= 0) {
          rowBuffers[gun_y-2][game_w/2] = '*'; colorBuffers[gun_y-2][game_w/2] = 93;
          rowBuffers[gun_y-1][game_w/2-1] = '\\'; colorBuffers[gun_y-1][game_w/2-1] = 93;
          rowBuffers[gun_y-1][game_w/2+1] = '/'; colorBuffers[gun_y-1][game_w/2+1] = 93;
        }
      }
      for (int gy = 0; gy < gun_h; gy++) {
        for (int gx = 0; gx < gun_w; gx++) {
          int sy = gun_y + gy, sx = gun_x + gx;
          if (sy >= 0 && sy < h && sx >= 0 && sx < game_w && gun_sprite[gy][gx] != ' ') {
            rowBuffers[sy][sx] = gun_sprite[gy][gx];
            colorBuffers[sy][sx] = 90; // Dark gray gun
          }
        }
      }
      
      // Side Panel HUD
      int scale_factor = h / 80;
      if (scale_factor < 1) scale_factor = 1;
      
      int hud_x = game_w + 3;
      if (hud_x + 30 < w) {
        int sy = h * 0.05f;
        draw_scaled_text(rowBuffers, colorBuffers, hud_x, sy, "DUCK HUNT 3D", scale_factor, w, h, 96); // Cyan
        
        sy += (5 * scale_factor) + (2 * scale_factor);
        char buf[64];
        snprintf(buf, sizeof(buf), "TIME: %02d", (int)game_timer);
        draw_scaled_text(rowBuffers, colorBuffers, hud_x, sy, buf, scale_factor, w, h, 97); // White
        
        sy += (5 * scale_factor) + (2 * scale_factor);
        snprintf(buf, sizeof(buf), "SCORE: %06d", score);
        draw_scaled_text(rowBuffers, colorBuffers, hud_x, sy, buf, scale_factor, w, h, 97);
        
        sy += (5 * scale_factor) + (2 * scale_factor);
        snprintf(buf, sizeof(buf), "HUNTED: %04d", kill_count);
        draw_scaled_text(rowBuffers, colorBuffers, hud_x, sy, buf, scale_factor, w, h, 97);
        
        sy += (5 * scale_factor) + (2 * scale_factor);
        snprintf(buf, sizeof(buf), "AMMO: [%d/8]", ammo);
        draw_scaled_text(rowBuffers, colorBuffers, hud_x, sy, buf, scale_factor, w, h, 97);
        
        if (reload_timer > 0.0f) {
          sy += (5 * scale_factor) + (2 * scale_factor);
          draw_scaled_text(rowBuffers, colorBuffers, hud_x, sy, "RELOADING", scale_factor, w, h, 91); // Red
        }
        
        // Draw controls at the bottom using scaled block font
        int ctrl_scale = scale_factor;
        
        sy += (10 * scale_factor); 
        
        if (sy + (5 * ctrl_scale) * 6 < h) { 
          draw_scaled_text(rowBuffers, colorBuffers, hud_x, sy, "--------------", ctrl_scale, w, h, 90); // Dark gray
          sy += (5 * ctrl_scale) + (2 * ctrl_scale);
          
          if (!mouse_locked) {
            draw_scaled_text(rowBuffers, colorBuffers, hud_x, sy, "PAUSED", ctrl_scale, w, h, 93); // Yellow
            sy += (5 * ctrl_scale) + (2 * ctrl_scale);
            draw_scaled_text(rowBuffers, colorBuffers, hud_x, sy, "PRESS ESC", ctrl_scale, w, h, 93);
          } else {
            draw_scaled_text(rowBuffers, colorBuffers, hud_x, sy, "CONTROLS:", ctrl_scale, w, h, 92); // Green
            sy += (5 * ctrl_scale) + (2 * ctrl_scale);
            draw_scaled_text(rowBuffers, colorBuffers, hud_x, sy, "WASD MOVE", ctrl_scale, w, h, 97);
            sy += (5 * ctrl_scale) + (2 * ctrl_scale);
            draw_scaled_text(rowBuffers, colorBuffers, hud_x, sy, "LMB SHOOT", ctrl_scale, w, h, 97);
            sy += (5 * ctrl_scale) + (2 * ctrl_scale);
            draw_scaled_text(rowBuffers, colorBuffers, hud_x, sy, "E RELOAD", ctrl_scale, w, h, 97);
            sy += (5 * ctrl_scale) + (2 * ctrl_scale);
            draw_scaled_text(rowBuffers, colorBuffers, hud_x, sy, "ESC PAUSE", ctrl_scale, w, h, 97);
          }
        }
      }
    }
    
    // Flush to terminal
    renderBuffer[0] = '\0';
    char* ptr = renderBuffer;
    ptr += sprintf(ptr, "\x1b[H");
    
    int current_color = -1; 
    for (int y = 0; y < h; y++) {
      for (int x = 0; x < w; x++) {
        int color = colorBuffers[y][x];
        if (color != current_color) {
          current_color = color;
          if (color == 0) ptr += sprintf(ptr, "\x1b[0m");
          else ptr += sprintf(ptr, "\x1b[%dm", color);
        }
        *ptr++ = rowBuffers[y][x];
      }
      *ptr++ = '\n';
    }
    ptr += sprintf(ptr, "\x1b[0m");
    fwrite(renderBuffer, 1, ptr - renderBuffer, stdout);
  }

  printf("\x1b[?25h"); 
  ShowCursor(TRUE);

  return 0;
}
