#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
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

const int width = 160;
const int height = 44;

float zBuffer[160 * 44];
char buffer[160 * 44];

// Camera controls
float camYaw = 0.0f;
float camPitch = -0.5f; // Negative = Look DOWN at the planets
float camX = 0.0f;
float camY = 30.0f;
float camZ = -70.0f;

// Simulation controls
float timeScale = 1.0f;
int paused = 0;

float K1 = 40.0f; // Projection scale

typedef struct {
  char name[32];
  float radius;
  float orbit_distance;
  float orbit_speed;
  float current_angle;
  int parent_index; // -1 for none
  int has_rings;
  float ring_inner;
  float ring_outer;
  float worldX;
  float worldY;
  float worldZ;
  int is_sun;
} CelestialBody;

#define NUM_BODIES 6
CelestialBody bodies[NUM_BODIES] = {
  // name, radius, orb_dist, orb_spd, curr_angle, parent, rings, ring_in, ring_out, wx, wy, wz, is_sun
  {"Sun",     6.0f,  0.0f,   0.0f,   0.0f, -1, 0, 0.0f,  0.0f,  0,0,0, 1},
  {"Earth",   2.0f,  20.0f,  0.02f,  0.0f,  0, 0, 0.0f,  0.0f,  0,0,0, 0},
  {"Moon",    0.8f,  4.0f,   0.1f,   0.0f,  1, 0, 0.0f,  0.0f,  0,0,0, 0},
  {"Mars",    1.5f,  30.0f,  0.015f, 2.0f,  0, 0, 0.0f,  0.0f,  0,0,0, 0},
  {"Jupiter", 4.0f,  50.0f,  0.008f, 4.0f,  0, 0, 0.0f,  0.0f,  0,0,0, 0},
  {"Saturn",  3.5f,  75.0f,  0.005f, 1.0f,  0, 1, 4.5f,  7.0f,  0,0,0, 0}
};

void projectAndDraw(float wx, float wy, float wz, char ch) {

  float cx = wx - camX;
  float cy = wy - camY;
  float cz = wz - camZ;

  float tx = cx * cos(camYaw) - cz * sin(camYaw);
  float tz = cx * sin(camYaw) + cz * cos(camYaw);

  float ty = cy * cos(camPitch) - tz * sin(camPitch);
  float finalZ = cy * sin(camPitch) + tz * cos(camPitch);

  if (finalZ > 0.1f) {
    float ooz = 1.0f / finalZ;
    int xp = (int)(width / 2.0f + K1 * tx * ooz * 2.0f);
    int yp = (int)(height / 2.0f - K1 * ty * ooz);

    if (xp >= 0 && xp < width && yp >= 0 && yp < height) {
      int idx = xp + yp * width;
      if (ooz > zBuffer[idx]) {
        zBuffer[idx] = ooz;
        buffer[idx] = ch;
      }
    }
  }
}

void drawSphere(int bodyIdx) {
  CelestialBody* b = &bodies[bodyIdx];
  float r = b->radius;
  
  // Calculate light vector pointing to the Sun (which is at 0,0,0)
  float Lx = -b->worldX;
  float Ly = -b->worldY;
  float Lz = -b->worldZ;
  float L_len = sqrt(Lx*Lx + Ly*Ly + Lz*Lz);
  if (L_len > 0) { Lx /= L_len; Ly /= L_len; Lz /= L_len; }

  for (float theta = 0; theta < 6.28f; theta += 0.15f) {
    for (float phi = 0; phi < 3.14f; phi += 0.15f) {
      // Local sphere coordinates
      float sx = r * sin(phi) * cos(theta);
      float sy = r * cos(phi);
      float sz = r * sin(phi) * sin(theta);

      // Normal is just (sx, sy, sz) normalized
      float nx = sx / r;
      float ny = sy / r;
      float nz = sz / r;

      char ch;
      if (b->is_sun) {
        ch = '@'; // Sun is always glowing
      } else {
        // Calculate illumination via dot product of Normal and Light vectors
        float LdotN = nx * Lx + ny * Ly + nz * Lz;
        
        if (LdotN > 0) {
          int luminance_index = (int)(LdotN * 11.0f);
          if (luminance_index > 11) luminance_index = 11;
          const char* shades = ".,-~:;=!*#$@";
          ch = shades[luminance_index];
        } else {
          ch = '.'; // Dark side
        }
      }

      projectAndDraw(b->worldX + sx, b->worldY + sy, b->worldZ + sz, ch);
    }
  }

  // Draw Rings if Saturn
  if (b->has_rings) {
    for (float ring_r = b->ring_inner; ring_r < b->ring_outer; ring_r += 0.3f) {
      for (float ring_theta = 0; ring_theta < 6.28f; ring_theta += 0.05f) {
        float rx = ring_r * cos(ring_theta);
        // Add a slight tilt to the ring
        float ry = ring_r * sin(ring_theta) * 0.3f;
        float rz = ring_r * sin(ring_theta);

        // Ring normal is roughly (0, 1, 0) relative to its tilt
        float nx = 0;
        float ny = 1.0f;
        float nz = 0;

        float LdotN = nx * Lx + ny * Ly + nz * Lz;
        // Make rings illuminated on both sides
        if (LdotN < 0) LdotN = -LdotN; 

        char ch = '.';
        if (LdotN > 0) {
          int luminance_index = (int)(LdotN * 8.0f) + 2; // Rings are a bit dimmer than max
          if (luminance_index > 11) luminance_index = 11;
          const char* shades = ".,-~:;=!*#$@";
          ch = shades[luminance_index];
        }
        
        // Darken the gaps in the ring
        if (ring_r > 5.5f && ring_r < 5.8f) ch = ' ';

        if (ch != ' ') {
          projectAndDraw(b->worldX + rx, b->worldY + ry, b->worldZ + rz, ch);
        }
      }
    }
  }
}

void updatePhysics() {
  for (int i = 0; i < NUM_BODIES; i++) {
    if (!paused) {
      bodies[i].current_angle += bodies[i].orbit_speed * timeScale;
    }

    float localX = bodies[i].orbit_distance * cos(bodies[i].current_angle);
    float localZ = bodies[i].orbit_distance * sin(bodies[i].current_angle);
    float localY = 0.0f;

    // Add slight inclination variation to orbits for 3D depth
    if (i == 2) localY = 2.0f * sin(bodies[i].current_angle); // Moon inclination
    if (i == 3) localY = -3.0f * cos(bodies[i].current_angle); // Mars inclination

    if (bodies[i].parent_index != -1) {
      bodies[i].worldX = bodies[bodies[i].parent_index].worldX + localX;
      bodies[i].worldY = bodies[bodies[i].parent_index].worldY + localY;
      bodies[i].worldZ = bodies[bodies[i].parent_index].worldZ + localZ;
    } else {
      bodies[i].worldX = localX;
      bodies[i].worldY = localY;
      bodies[i].worldZ = localZ;
    }
  }
}

int main() {
  printf("\x1b[2J");

  while (1) {
    if (_kbhit()) {
      char key = _getch();
      float speed = 3.0f;
      float fwdX = sin(camYaw) * cos(camPitch);
      float fwdY = sin(camPitch);
      float fwdZ = cos(camYaw) * cos(camPitch);
      float rightX = cos(camYaw);
      float rightZ = -sin(camYaw);

      if (key == 'w') { camX += fwdX * speed; camY += fwdY * speed; camZ += fwdZ * speed; }
      if (key == 's') { camX -= fwdX * speed; camY -= fwdY * speed; camZ -= fwdZ * speed; }
      if (key == 'a') { camX -= rightX * speed; camZ -= rightZ * speed; }
      if (key == 'd') { camX += rightX * speed; camZ += rightZ * speed; }
      if (key == 'e') { camY += speed; } // Move Up
      if (key == 'q') { camY -= speed; } // Move Down
      if (key == 'j') camYaw -= 0.1f;
      if (key == 'l') camYaw += 0.1f;
      if (key == 'i') camPitch += 0.1f;
      if (key == 'k') camPitch -= 0.1f;
      if (key == '[') timeScale *= 0.5f;
      if (key == ']') timeScale *= 2.0f;
      if (key == 'p') paused = !paused;
      if (key == 27) break; // ESC to exit
      if (camPitch > 1.5f) camPitch = 1.5f;
      if (camPitch < -1.5f) camPitch = -1.5f;
    }

    updatePhysics();

    memset(buffer, ' ', width * height);
    for (int i = 0; i < width * height; i++) {
      zBuffer[i] = 0.0f;
    }

    // Draw grid plane for reference
    for (int x = -100; x <= 100; x += 10) {
      for (int z = -100; z <= 100; z += 10) {
        projectAndDraw(x, -10.0f, z, '.');
      }
    }

    // Draw celestial bodies
    for (int i = 0; i < NUM_BODIES; i++) {
      drawSphere(i);
    }

    printf("\x1b[H");
    for (int i = 0; i < width * height; i++) {
      putchar(i % width == width - 1 ? '\n' : buffer[i]);
    }
    
    printf("Controls: [WASD] Move | [QE] Up/Down | [IJKL] Look | [ [ / ] ] Speed: %.2fx | [P] Pause | [ESC] Exit\n", timeScale);
    printf("Camera Pos: (%.1f, %.1f, %.1f) | Yaw: %.2f | Pitch: %.2f\n", camX, camY, camZ, camYaw, camPitch);

    usleep(30000); // ~33 FPS
  }

  return 0;
}
