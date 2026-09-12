#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
int _kbhit(void) {
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
  if(ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }
  return 0;
}
int _getch(void) {
  struct termios oldt, newt;
  int ch;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  ch = getchar();
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return ch;
}
#endif

void getTerminalSize(int *w, int *h) {
#ifdef _WIN32
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
    *w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    *h = csbi.srWindow.Bottom - csbi.srWindow.Top;
  } else {
    *w = 160;
    *h = 44;
  }
#else
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
    *w = ws.ws_col;
    *h = ws.ws_row - 1;
  } else {
    *w = 160;
    *h = 44;
  }
#endif
  if (*w < 40) *w = 40;
  if (*h < 15) *h = 15;
  *w = (int)(*w * 0.95f);
  *h = (int)(*h * 0.95f);
}

typedef struct {
  int logical_pos[3];
  int logical_rot[3][3];
  float visual_pos[3];
  float visual_rot[3][3];
  int base_colors[6];
} Cubie;

Cubie cubies[27];

int color_codes[6] = {41, 45, 47, 43, 42, 44};
int internal_color = 0; 

int width = 0, height = 0;
float *zBuffer = NULL;
char *char_buffer = NULL;
int *color_buffer = NULL;
char *renderBuffer = NULL;
float K1 = 40;
float distanceFromCam = 25.0f;

float camA = 0.5f, camB = -0.5f;

typedef enum { IDLE, ANIMATING } State;
State current_state = IDLE;

int target_axis = 0; 
int target_slice = 0; 
int target_dir = 1; 
float current_angle = 0.0f;
float target_angle = 0.0f;
float anim_speed = 0.15f;
int total_moves = 0;

int get_glyph(char c) {
    if (c >= 'a' && c <= 'z') c = c - 32; // Uppercase
    switch (c) {
    case 'A': return 11245;
    case 'B': return 27630;
    case 'C': return 14627;
    case 'D': return 27502;
    case 'E': return 31143;
    case 'F': return 31140;
    case 'G': return 14699;
    case 'H': return 23533;
    case 'I': return 29847;
    case 'J': return 4714;
    case 'K': return 23861;
    case 'L': return 18727;
    case 'M': return 24429;
    case 'N': return 31597;
    case 'O': return 11114;
    case 'P': return 27556;
    case 'Q': return 11097;
    case 'R': return 27565;
    case 'S': return 14478;
    case 'T': return 29842;
    case 'U': return 23403;
    case 'V': return 23402;
    case 'W': return 23421;
    case 'X': return 23213;
    case 'Y': return 23186;
    case 'Z': return 29351;
    case '0': return 11114;
    case '1': return 11415;
    case '2': return 29671;
    case '3': return 29391;
    case '4': return 23497;
    case '5': return 31183;
    case '6': return 14827;
    case '7': return 29330;
    case '8': return 15083;
    case '9': return 15051;
    case '[': return 26918;
    case ']': return 12875;
    case '/': return 4772;
    case '+': return 1488;
    case '-': return 448;
    case '=': return 3640;
    case ':': return 1040;
    case '|': return 9362;
    case ' ': return 0;
    case '\'': return 9216;
    default: return 0;
    }
}

void draw_block_char(int start_x, int start_y, char c, int scale, int color_ansi) {
    int glyph = get_glyph(c);
    if (glyph == 0) return; // space
    
    // 3x5 grid, MSB is top-left
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 3; col++) {
            int bit_idx = 14 - (row * 3 + col);
            int bit = (glyph >> bit_idx) & 1;
            
            if (bit) {
                // draw a scale x scale block
                for (int dy = 0; dy < scale; dy++) {
                    for (int dx = 0; dx < scale; dx++) {
                        int py = start_y + row * scale + dy;
                        int px = start_x + col * scale + dx;
                        
                        if (px >= 0 && px < width && py >= 0 && py < height) {
                            int idx = px + py * width;
                            char_buffer[idx] = ' ';
                            color_buffer[idx] = color_ansi;
                        }
                    }
                }
            }
        }
    }
}

void draw_block_text(int start_x, int start_y, const char* str, int scale, int color_ansi) {
    int len = strlen(str);
    int cursor_x = start_x;
    for (int i = 0; i < len; i++) {
        draw_block_char(cursor_x, start_y, str[i], scale, color_ansi);
        cursor_x += (3 * scale) + scale; // width + 1 block spacing
    }
}

void init_matrix(int mat[3][3]) {
  for(int i=0; i<3; i++)
    for(int j=0; j<3; j++)
      mat[i][j] = (i == j) ? 1 : 0;
}

void init_matrix_f(float mat[3][3]) {
  for(int i=0; i<3; i++)
    for(int j=0; j<3; j++)
      mat[i][j] = (i == j) ? 1.0f : 0.0f;
}

void mat_mul_int(int a[3][3], int b[3][3], int out[3][3]) {
  for(int i=0; i<3; i++) {
    for(int j=0; j<3; j++) {
      out[i][j] = 0;
      for(int k=0; k<3; k++) out[i][j] += a[i][k] * b[k][j];
    }
  }
}

void vec_mul_int(int a[3][3], int v[3], int out[3]) {
  for(int i=0; i<3; i++) {
    out[i] = 0;
    for(int j=0; j<3; j++) out[i] += a[i][j] * v[j];
  }
}

void mat_mul_f(float a[3][3], float b[3][3], float out[3][3]) {
  for(int i=0; i<3; i++) {
    for(int j=0; j<3; j++) {
      out[i][j] = 0.0f;
      for(int k=0; k<3; k++) out[i][j] += a[i][k] * b[k][j];
    }
  }
}

void get_rot_matrix_int(int axis, int dir, int out[3][3]) {
  init_matrix(out);
  if (axis == 0) { // X
    out[1][1] = 0; out[1][2] = -dir;
    out[2][1] = dir; out[2][2] = 0;
  } else if (axis == 1) { // Y
    out[0][0] = 0; out[0][2] = dir;
    out[2][0] = -dir; out[2][2] = 0;
  } else if (axis == 2) { // Z
    out[0][0] = 0; out[0][1] = -dir;
    out[1][0] = dir; out[1][1] = 0;
  }
}

void get_rot_matrix_f(int axis, float angle, float out[3][3]) {
  init_matrix_f(out);
  float c = cos(angle);
  float s = sin(angle);
  if (axis == 0) { // X
    out[1][1] = c; out[1][2] = -s;
    out[2][1] = s; out[2][2] = c;
  } else if (axis == 1) { // Y
    out[0][0] = c; out[0][2] = s;
    out[2][0] = -s; out[2][2] = c;
  } else if (axis == 2) { // Z
    out[0][0] = c; out[0][1] = -s;
    out[1][0] = s; out[1][1] = c;
  }
}

void apply_logical_move(int axis, int slice, int dir) {
  int R[3][3];
  get_rot_matrix_int(axis, dir, R);
  
  for (int i = 0; i < 27; i++) {
    if (cubies[i].logical_pos[axis] == slice) {
      int new_pos[3];
      vec_mul_int(R, cubies[i].logical_pos, new_pos);
      cubies[i].logical_pos[0] = new_pos[0];
      cubies[i].logical_pos[1] = new_pos[1];
      cubies[i].logical_pos[2] = new_pos[2];
      
      int new_rot[3][3];
      mat_mul_int(R, cubies[i].logical_rot, new_rot);
      memcpy(cubies[i].logical_rot, new_rot, sizeof(new_rot));
    }
  }
}

void sync_visual_state() {
  for (int i = 0; i < 27; i++) {
    cubies[i].visual_pos[0] = (float)cubies[i].logical_pos[0];
    cubies[i].visual_pos[1] = (float)cubies[i].logical_pos[1];
    cubies[i].visual_pos[2] = (float)cubies[i].logical_pos[2];
    
    for (int r = 0; r < 3; r++) {
      for (int c = 0; c < 3; c++) {
        cubies[i].visual_rot[r][c] = (float)cubies[i].logical_rot[r][c];
      }
    }
  }
}

void init_cubies() {
  int idx = 0;
  for(int x = -1; x <= 1; x++) {
    for(int y = -1; y <= 1; y++) {
      for(int z = -1; z <= 1; z++) {
        cubies[idx].logical_pos[0] = x;
        cubies[idx].logical_pos[1] = y;
        cubies[idx].logical_pos[2] = z;
        init_matrix(cubies[idx].logical_rot);
        
        cubies[idx].base_colors[0] = (x == 1) ? color_codes[0] : internal_color;
        cubies[idx].base_colors[1] = (x == -1) ? color_codes[1] : internal_color;
        cubies[idx].base_colors[2] = (y == -1) ? color_codes[2] : internal_color;
        cubies[idx].base_colors[3] = (y == 1) ? color_codes[3] : internal_color;
        cubies[idx].base_colors[4] = (z == -1) ? color_codes[4] : internal_color;
        cubies[idx].base_colors[5] = (z == 1) ? color_codes[5] : internal_color;
        
        idx++;
      }
    }
  }
  sync_visual_state();
  total_moves = 0;
}

void scramble() {
  srand((unsigned int)time(NULL));
  for (int i = 0; i < 25; i++) {
    int axis = rand() % 3;
    int slice = (rand() % 3) - 1;
    int dir = (rand() % 2 == 0) ? 1 : -1;
    apply_logical_move(axis, slice, dir);
  }
  sync_visual_state();
  total_moves = 0;
}

void start_animation(int axis, int slice, int dir) {
  if (current_state != IDLE) return;
  target_axis = axis;
  target_slice = slice;
  target_dir = dir;
  current_angle = 0.0f;
  target_angle = (float)dir * 3.14159265f / 2.0f;
  current_state = ANIMATING;
}

void update_animation() {
  if (current_state == ANIMATING) {
    float step = anim_speed * (target_dir > 0 ? 1 : -1);
    current_angle += step;
    
    if ((target_dir > 0 && current_angle >= target_angle) || 
        (target_dir < 0 && current_angle <= target_angle)) {
      current_angle = target_angle;
      apply_logical_move(target_axis, target_slice, target_dir);
      sync_visual_state();
      current_state = IDLE;
      total_moves++;
      return;
    }
    
    float R[3][3];
    get_rot_matrix_f(target_axis, current_angle, R);
    
    for (int i = 0; i < 27; i++) {
      if (cubies[i].logical_pos[target_axis] == target_slice) {
        float lp[3] = {(float)cubies[i].logical_pos[0], (float)cubies[i].logical_pos[1], (float)cubies[i].logical_pos[2]};
        cubies[i].visual_pos[0] = R[0][0]*lp[0] + R[0][1]*lp[1] + R[0][2]*lp[2];
        cubies[i].visual_pos[1] = R[1][0]*lp[0] + R[1][1]*lp[1] + R[1][2]*lp[2];
        cubies[i].visual_pos[2] = R[2][0]*lp[0] + R[2][1]*lp[1] + R[2][2]*lp[2];
        
        float lr[3][3];
        for (int r = 0; r < 3; r++)
          for (int c = 0; c < 3; c++)
            lr[r][c] = (float)cubies[i].logical_rot[r][c];
            
        mat_mul_f(R, lr, cubies[i].visual_rot);
      }
    }
  }
}

float edgeFunction(float ax, float ay, float bx, float by, float px, float py) {
  return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
}

void transform_point(float p[3], float rot[3][3], float pos[3], float out[3]) {
  out[0] = rot[0][0]*p[0] + rot[0][1]*p[1] + rot[0][2]*p[2] + pos[0];
  out[1] = rot[1][0]*p[0] + rot[1][1]*p[1] + rot[1][2]*p[2] + pos[1];
  out[2] = rot[2][0]*p[0] + rot[2][1]*p[1] + rot[2][2]*p[2] + pos[2];
}

void apply_camera(float p[3], float out[3]) {
  float y1 = p[1]*cos(camA) - p[2]*sin(camA);
  float z1 = p[1]*sin(camA) + p[2]*cos(camA);
  
  float x2 = p[0]*cos(camB) + z1*sin(camB);
  float z2 = -p[0]*sin(camB) + z1*cos(camB);
  
  out[0] = x2;
  out[1] = y1;
  out[2] = z2;
}

void rasterize_triangle(float v0[3], float v1[3], float v2[3], int color) {
  if (color == internal_color) return;
  
  float p0[3], p1[3], p2[3];
  apply_camera(v0, p0);
  apply_camera(v1, p1);
  apply_camera(v2, p2);
  
  p0[2] += distanceFromCam;
  p1[2] += distanceFromCam;
  p2[2] += distanceFromCam;
  
  if (p0[2] <= 0.1f || p1[2] <= 0.1f || p2[2] <= 0.1f) return;
  
  float ooz0 = 1.0f / p0[2];
  float ooz1 = 1.0f / p1[2];
  float ooz2 = 1.0f / p2[2];
  
  float center_x = width * 0.325f;
  float center_y = height * 0.50f;
  
  float x0 = center_x + (p0[0] * ooz0) * K1 * 2.0f;
  float y0 = center_y + (p0[1] * ooz0) * K1;
  
  float x1 = center_x + (p1[0] * ooz1) * K1 * 2.0f;
  float y1 = center_y + (p1[1] * ooz1) * K1;
  
  float x2 = center_x + (p2[0] * ooz2) * K1 * 2.0f;
  float y2 = center_y + (p2[1] * ooz2) * K1;
  
  int min_x = (int)fmin(x0, fmin(x1, x2));
  int max_x = (int)fmax(x0, fmax(x1, x2));
  int min_y = (int)fmin(y0, fmin(y1, y2));
  int max_y = (int)fmax(y0, fmax(y1, y2));
  
  if (min_x < 0) min_x = 0;
  if (max_x >= width) max_x = width - 1;
  if (min_y < 0) min_y = 0;
  if (max_y >= height) max_y = height - 1;
  
  float area = edgeFunction(x0, y0, x1, y1, x2, y2);
  if (area == 0.0f) return;
  bool is_ccw = area > 0;
  
  for (int y = min_y; y <= max_y; y++) {
    for (int x = min_x; x <= max_x; x++) {
      float px = (float)x + 0.5f;
      float py = (float)y + 0.5f;
      
      float w0 = edgeFunction(x1, y1, x2, y2, px, py);
      float w1 = edgeFunction(x2, y2, x0, y0, px, py);
      float w2 = edgeFunction(x0, y0, x1, y1, px, py);
      
      if ((is_ccw && w0 >= 0 && w1 >= 0 && w2 >= 0) || (!is_ccw && w0 <= 0 && w1 <= 0 && w2 <= 0)) {
        w0 /= area;
        w1 /= area;
        w2 /= area;
        
        float ooz = w0 * ooz0 + w1 * ooz1 + w2 * ooz2;
        int idx = x + y * width;
        if (ooz > zBuffer[idx]) {
          zBuffer[idx] = ooz;
          char_buffer[idx] = ' '; 
          color_buffer[idx] = color;
        }
      }
    }
  }
}

void draw_minimap_and_hud() {
  int get_face_color(Cubie *c, int wn[3]) {
    int ln[3];
    for (int i = 0; i < 3; i++) {
      ln[i] = c->logical_rot[0][i]*wn[0] + c->logical_rot[1][i]*wn[1] + c->logical_rot[2][i]*wn[2];
    }
    if (ln[0] == 1) return c->base_colors[0];
    if (ln[0] == -1) return c->base_colors[1];
    if (ln[1] == -1) return c->base_colors[2];
    if (ln[1] == 1) return c->base_colors[3];
    if (ln[2] == -1) return c->base_colors[4];
    if (ln[2] == 1) return c->base_colors[5];
    return internal_color;
  }
  
  int map[9][12];
  memset(map, 0, sizeof(map));
  
  for (int i = 0; i < 27; i++) {
    Cubie *c = &cubies[i];
    int x = c->logical_pos[0];
    int y = c->logical_pos[1];
    int z = c->logical_pos[2];
    
    int cx = x + 1; 
    int cy = y + 1;
    int cz = z + 1;
    
    if (y == -1) { int wn[3] = {0, -1, 0}; map[cz][3 + cx] = get_face_color(c, wn); }
    if (y == 1) { int wn[3] = {0, 1, 0}; map[6 + (2 - cz)][3 + cx] = get_face_color(c, wn); }
    if (z == -1) { int wn[3] = {0, 0, -1}; map[3 + cy][3 + cx] = get_face_color(c, wn); }
    if (z == 1) { int wn[3] = {0, 0, 1}; map[3 + cy][9 + (2 - cx)] = get_face_color(c, wn); }
    if (x == -1) { int wn[3] = {-1, 0, 0}; map[3 + cy][(2 - cz)] = get_face_color(c, wn); }
    if (x == 1) { int wn[3] = {1, 0, 0}; map[3 + cy][6 + cz] = get_face_color(c, wn); }
  }
  
  int ui_scale = height / 25;
  if (ui_scale < 1) ui_scale = 1;
  
  int cw = ui_scale * 3; 
  int ch = ui_scale;     
  
  int total_width = 12 * cw + 13;
  int hud_x = width - total_width - 5;
  if (hud_x < (int)(width * 0.40f)) hud_x = (int)(width * 0.40f);
  
  int total_height = 9 * ch + 10;
  int hud_y = (height - total_height) / 2;
  if (hud_y < 2) hud_y = 2;
  
  int font_scale = (ui_scale > 1) ? ui_scale / 2 : 1;
  if (font_scale < 1) font_scale = 1;
  
  int title_w = strlen("3D RUBIK'S CUBE") * ((3 * font_scale) + font_scale) - font_scale;
  int title_x = hud_x + (total_width - title_w) / 2;
  if (title_x < 0) title_x = 0;
  draw_block_text(title_x, hud_y - (5 * font_scale) - 2, "3D RUBIK'S CUBE", font_scale, 47); // white background
  
  void draw_hline(int start_x, int start_y, int cols) {
    for (int c = 0; c < cols; c++) {
      char_buffer[start_x + c * (cw + 1) + start_y * width] = '+';
      for (int k = 1; k <= cw; k++) {
        char_buffer[start_x + c * (cw + 1) + k + start_y * width] = '-';
      }
    }
    char_buffer[start_x + cols * (cw + 1) + start_y * width] = '+';
  }
  
  for (int block_row = 0; block_row < 3; block_row++) {
    for (int block_col = 0; block_col < 4; block_col++) {
      
      if ((block_col == 1) || (block_row == 1)) {
        int start_y = hud_y + block_row * (3 * ch + 1);
        int start_x = hud_x + block_col * (3 * cw + 1);
        
        if (block_row != 1 || block_col == 0) {
          draw_hline(start_x, start_y, 3);
        }
        
        char label = ' ';
        if (block_row == 0 && block_col == 1) label = 'U';
        else if (block_row == 2 && block_col == 1) label = 'D';
        else if (block_row == 1 && block_col == 0) label = 'L';
        else if (block_row == 1 && block_col == 1) label = 'F';
        else if (block_row == 1 && block_col == 2) label = 'R';
        else if (block_row == 1 && block_col == 3) label = 'B';
        
        for (int r = 0; r < 3; r++) {
          for (int line = 0; line < ch; line++) {
            int row_y = start_y + 1 + r * (ch + 1) + line;
            for (int c = 0; c < 3; c++) {
              int cell_x = start_x + c * (cw + 1);
              char_buffer[cell_x + row_y * width] = '|';
              
              int map_val = map[block_row * 3 + r][block_col * 3 + c];
              
              for (int k = 1; k <= cw; k++) {
                int cx = cell_x + k;
                if (cx >= 0 && cx < width && row_y >= 0 && row_y < height) {
                  int idx = cx + row_y * width;
                  if (line == ch / 2 && k == cw / 2 + 1) {
                    if (r == 1 && c == 1) {
                      char_buffer[idx] = label;
                    } else {
                      char_buffer[idx] = ' ';
                    }
                  } else {
                    char_buffer[idx] = ' '; 
                  }
                  color_buffer[idx] = map_val;
                }
              }
            }
            char_buffer[start_x + 3 * (cw + 1) + row_y * width] = '|';
          }
          draw_hline(start_x, start_y + 1 + r * (ch + 1) + ch, 3);
        }
      }
    }
  }
  
  char moves_str[64];
  sprintf(moves_str, "MOVES: %d", total_moves);
  
  if (height > 70) {
    char *compact1 = "U/D/L/R/F/B: CW    SHIFT: CCW";
    char *compact2 = "ARROWS: ORBIT    +/-: SCALE    SPACE: RESET";
    
    // Prevent overlapping by ensuring font_scale allows compact2 (longest string) to fit in width
    int max_len = strlen(compact2);
    int max_fs = (width - 10) / (max_len * 4);
    if (font_scale > max_fs) font_scale = max_fs;
    if (font_scale < 1) font_scale = 1;
    
    int char_w = (3 * font_scale) + font_scale;
    int c1_w = strlen(compact1) * char_w - font_scale;
    int c2_w = strlen(compact2) * char_w - font_scale;
    
    int bar_y2 = height - (5 * font_scale) - 2;
    int bar_y1 = bar_y2 - (5 * font_scale) - 2;
    
    int compact_x1 = (width - c1_w) / 2;
    int compact_x2 = (width - c2_w) / 2;
    if (compact_x1 < 0) compact_x1 = 0;
    if (compact_x2 < 0) compact_x2 = 0;
    
    draw_block_text(compact_x1, bar_y1, compact1, font_scale, 47);
    draw_block_text(compact_x2, bar_y2, compact2, font_scale, 47);
    
    // Draw moves at the top right to completely avoid overlap
    int moves_w = strlen(moves_str) * char_w - font_scale;
    int moves_x = width - moves_w - 5;
    if (moves_x < 0) moves_x = 0;
    draw_block_text(moves_x, 2, moves_str, font_scale, 47);
  } else {
    // Normal layout - clamp scale to fit sidebar
    int max_fs = (width - hud_x - 5) / (22 * 4);
    if (font_scale > max_fs) font_scale = max_fs;
    if (font_scale < 1) font_scale = 1;
    
    int ctrl_y = hud_y + total_height + 2;
    draw_block_text(hud_x, ctrl_y, "CONTROLS", font_scale, 47);
    ctrl_y += (5 * font_scale) + 2;
    draw_block_text(hud_x, ctrl_y, "[U/D/L/R/F/B] TURN CW", font_scale, 47);
    ctrl_y += (5 * font_scale) + 2;
    draw_block_text(hud_x, ctrl_y, "[SHIFT+KEY]   TURN CCW", font_scale, 47);
    ctrl_y += (5 * font_scale) + 2;
    draw_block_text(hud_x, ctrl_y, "[ARROWS]      ORBIT", font_scale, 47);
    ctrl_y += (5 * font_scale) + 2;
    draw_block_text(hud_x, ctrl_y, "[+/-]         SCALE", font_scale, 47);
    ctrl_y += (5 * font_scale) + 2;
    draw_block_text(hud_x, ctrl_y, "[SPACE]       RESET", font_scale, 47);
    ctrl_y += (5 * font_scale) + 2;
    
    draw_block_text(hud_x, ctrl_y + 2, moves_str, font_scale, 47);
  }
}

void render_frame() {
  memset(char_buffer, ' ', width * height);
  memset(color_buffer, 0, width * height * sizeof(int));
  memset(zBuffer, 0, width * height * sizeof(float));
  
  for (int i = 0; i < 27; i++) {
    float r = 0.48f; 
    
    float faces[6][4][3] = {
      {{r, -r, -r}, {r, r, -r}, {r, r, r}, {r, -r, r}},   
      {{-r, -r, r}, {-r, r, r}, {-r, r, -r}, {-r, -r, -r}},
      {{-r, -r, r}, {-r, -r, -r}, {r, -r, -r}, {r, -r, r}},
      {{-r, r, -r}, {-r, r, r}, {r, r, r}, {r, r, -r}},    
      {{-r, -r, -r}, {-r, r, -r}, {r, r, -r}, {r, -r, -r}},
      {{r, -r, r}, {r, r, r}, {-r, r, r}, {-r, -r, r}}     
    };
    
    for (int f = 0; f < 6; f++) {
      if (cubies[i].base_colors[f] == internal_color) continue;
      
      float v[4][3];
      for (int j = 0; j < 4; j++) {
        transform_point(faces[f][j], cubies[i].visual_rot, cubies[i].visual_pos, v[j]);
      }
      
      rasterize_triangle(v[0], v[1], v[2], cubies[i].base_colors[f]);
      rasterize_triangle(v[0], v[2], v[3], cubies[i].base_colors[f]);
    }
  }
  
  draw_minimap_and_hud();
  
  int p = 0;
  renderBuffer[p++] = '\x1b';
  renderBuffer[p++] = '[';
  renderBuffer[p++] = 'H';
  
  int current_color = 0;
  
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int idx = x + y * width;
      int color = color_buffer[idx];
      char c = char_buffer[idx];
      
      if (color != current_color) {
        if (color == 0) {
          p += sprintf(&renderBuffer[p], "\x1b[0m");
        } else {
          p += sprintf(&renderBuffer[p], "\x1b[%dm", color);
        }
        current_color = color;
      }
      renderBuffer[p++] = c;
    }
    if (current_color != 0) {
      p += sprintf(&renderBuffer[p], "\x1b[0m");
      current_color = 0;
    }
    renderBuffer[p++] = '\n';
  }
  
  fwrite(renderBuffer, 1, p, stdout);
  fflush(stdout);
}

int main() {
#ifdef _WIN32
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD dwMode = 0;
  GetConsoleMode(hOut, &dwMode);
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(hOut, dwMode);
#endif
  setvbuf(stdout, NULL, _IONBF, 0);
  
  init_cubies();
  scramble();
  
  printf("\x1b[2J");
  while (1) {
    if (_kbhit()) {
      char key = _getch();
      if (key == 27) break; // Esc
      
      if (key == ' ') { init_cubies(); scramble(); }
      
      if (key == '=' || key == '+') distanceFromCam = fmaxf(15.0f, distanceFromCam - 2.0f);
      if (key == '-' || key == '_') distanceFromCam = fminf(80.0f, distanceFromCam + 2.0f);
      
      int dir = 1;
      char lower = key;
      if (key >= 'A' && key <= 'Z') {
        dir = -1;
        lower = key + 32;
      }
      
      if (lower == 'r') start_animation(0, 1, dir);
      if (lower == 'l') start_animation(0, -1, -dir); 
      if (lower == 'd') start_animation(1, 1, dir); 
      if (lower == 'u') start_animation(1, -1, -dir);
      if (lower == 'b') start_animation(2, 1, dir); 
      if (lower == 'f') start_animation(2, -1, -dir);
      
#ifdef _WIN32
      if (key == -32 || key == 224) { 
        key = _getch();
        if (key == 72) camA -= 0.1f; 
        if (key == 80) camA += 0.1f; 
        if (key == 75) camB -= 0.1f; 
        if (key == 77) camB += 0.1f; 
      }
#else
      if (key == '\033') { 
        _getch(); 
        char seq = _getch();
        if (seq == 'A') camA -= 0.1f; 
        if (seq == 'B') camA += 0.1f; 
        if (seq == 'C') camB += 0.1f; 
        if (seq == 'D') camB -= 0.1f; 
      }
#endif
    }
    
    update_animation();
    
    int new_w, new_h;
    getTerminalSize(&new_w, &new_h);
    if (new_w != width || new_h != height) {
      width = new_w;
      height = new_h;
      zBuffer = (float *)realloc(zBuffer, width * height * sizeof(float));
      char_buffer = (char *)realloc(char_buffer, width * height * sizeof(char));
      color_buffer = (int *)realloc(color_buffer, width * height * sizeof(int));
      renderBuffer = (char *)realloc(renderBuffer, 10 + height * (width * 15 + 1) + 256);
      K1 = height * 0.9f;
      printf("\x1b[2J");
    }
    
    render_frame();
    usleep(16000); 
  }
  
  printf("\x1b[0m\x1b[2J");
  return 0;
}
