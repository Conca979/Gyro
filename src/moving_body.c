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

float camX = 0, camY = 0, camZ = 0;
float camYaw = 0.0f, camPitch = 0.3f;
float camDist = 35.0f;
float K1 = 50.0f;

float robotX = 0, robotY = 0, robotZ = 0;
float robotYaw = 0.0f;

void projectWorld(float wx, float wy, float wz, char ch) {
  float cx = wx - camX;
  float cy = wy - camY;
  float cz = wz - camZ;
  
  float sY = sin(-camYaw), cY = cos(-camYaw);
  float cx2 = cx * cY + cz * sY;
  float cz2 = -cx * sY + cz * cY;
  cx = cx2; cz = cz2;
  
  float sP = sin(-camPitch), cP = cos(-camPitch);
  float cy2 = cy * cP - cz * sP;
  float cz3 = cy * sP + cz * cP;
  cy = cy2; cz = cz3;
  
  if (cz <= 0.1f) return;
  
  float ooz = 1.0f / cz;
  int xp = (int)(width / 2.0f + K1 * ooz * cx * 2.0f);
  int yp = (int)(height / 2.0f - K1 * ooz * cy); // Y inverted for terminal
  
  int idx = xp + yp * width;
  if (xp >= 0 && xp < width && yp >= 0 && yp < height) {
    if (ooz > zBuffer[idx]) {
      zBuffer[idx] = ooz;
      buffer[idx] = ch;
    }
  }
}

void transformAndDraw(float lx, float ly, float lz, float jx, float jy, float jz, float rotX, float rotY, float rotZ, char ch) {
  // 1. Joint rotation X
  float ty = ly * cos(rotX) - lz * sin(rotX);
  float tz = ly * sin(rotX) + lz * cos(rotX);
  ly = ty; lz = tz;
  
  // 2. Attach to body
  float bx = lx + jx;
  float by = ly + jy;
  float bz = lz + jz;
  
  // 3. Robot rotation (Yaw)
  float rx = bx * cos(robotYaw) + bz * sin(robotYaw);
  float rz = -bx * sin(robotYaw) + bz * cos(robotYaw);
  bx = rx; bz = rz;
  
  // 4. World position
  float wx = bx + robotX;
  float wy = by + robotY;
  float wz = bz + robotZ;
  
  projectWorld(wx, wy, wz, ch);
}

void drawBox(
  float bw, float bh, float bd,
  float ox, float oy, float oz, 
  float jx, float jy, float jz, 
  float rotX, float rotY, float rotZ, 
  char ch
) {
  float step = 0.5f;
  for (float x = -bw/2; x <= bw/2; x += step) {
    for (float y = -bh/2; y <= bh/2; y += step) {
      transformAndDraw(x + ox, y + oy, -bd/2 + oz, jx, jy, jz, rotX, rotY, rotZ, ch);
      transformAndDraw(x + ox, y + oy, bd/2 + oz, jx, jy, jz, rotX, rotY, rotZ, ch);
    }
  }
  for (float z = -bd/2; z <= bd/2; z += step) {
    for (float y = -bh/2; y <= bh/2; y += step) {
      transformAndDraw(-bw/2 + ox, y + oy, z + oz, jx, jy, jz, rotX, rotY, rotZ, ch);
      transformAndDraw(bw/2 + ox, y + oy, z + oz, jx, jy, jz, rotX, rotY, rotZ, ch);
    }
  }
  for (float x = -bw/2; x <= bw/2; x += step) {
    for (float z = -bd/2; z <= bd/2; z += step) {
      transformAndDraw(x + ox, -bh/2 + oy, z + oz, jx, jy, jz, rotX, rotY, rotZ, ch);
      transformAndDraw(x + ox, bh/2 + oy, z + oz, jx, jy, jz, rotX, rotY, rotZ, ch);
    }
  }
}

int main() {
  printf("\x1b[2J");
  
  float walkPhase = 0.0f;
  float currentLimbAngle = 0.0f;
  float targetRobotYaw = robotYaw;
  int timeSinceLastMove = 100;
  
  while (1) {
    int isMoving = 0;
    float dz = 0, dx = 0;

    if (_kbhit()) {
      char key = _getch();
      if (key == 27) break; // Esc
      if (key == 'i' || key == 'I') camPitch += 0.1f;
      if (key == 'k' || key == 'K') camPitch -= 0.1f;
      if (key == 'j' || key == 'J') camYaw -= 0.1f;
      if (key == 'l' || key == 'L') camYaw += 0.1f;
      
      float moveSpeed = 1.0f;
      if (key == 'w' || key == 'W') dz = moveSpeed;
      if (key == 's' || key == 'S') dz = -moveSpeed;
      if (key == 'a' || key == 'A') dx = -moveSpeed;
      if (key == 'd' || key == 'D') dx = moveSpeed;
      
      if (key == '=' || key == '+') camDist -= 2.0f;
      if (key == '-' || key == '_') camDist += 2.0f;
    }

    if (dz != 0 || dx != 0) {
      isMoving = 1;
      float fx = sin(camYaw), fz = cos(camYaw);
      float rx = cos(camYaw), rz = -sin(camYaw);
      
      float worldDx = fx * dz + rx * dx;
      float worldDz = fz * dz + rz * dx;
      
      robotX += worldDx;
      robotZ += worldDz;
      
      targetRobotYaw = atan2(worldDx, worldDz);
    }

    if (isMoving) timeSinceLastMove = 0;
    else timeSinceLastMove++;

    float targetLimbAngle = 0.0f;
    if (timeSinceLastMove < 3) {
      walkPhase += 0.35f;
      targetLimbAngle = sin(walkPhase) * 1.2f;
    } else {
      walkPhase = 0.0f;
    }
    
    currentLimbAngle += (targetLimbAngle - currentLimbAngle) * 0.3f;
    
    float diff = targetRobotYaw - robotYaw;
    while (diff > 3.14159f) diff -= 6.28318f;
    while (diff < -3.14159f) diff += 6.28318f;
    robotYaw += diff * 0.15f;

    // Camera placement
    camX = robotX - sin(camYaw) * camDist * cos(camPitch);
    camY = robotY + sin(camPitch) * camDist;
    camZ = robotZ - cos(camYaw) * camDist * cos(camPitch);

    memset(buffer, ' ', width * height);
    memset(zBuffer, 0, width * height * sizeof(float));

    // Draw Floor Grid
    for (float fx = -60; fx <= 60; fx += 3) {
      for (float fz = -60; fz <= 60; fz += 3) {
        projectWorld(fx, -8, fz, '.');
      }
    }

    // Draw Robot Body
    // Torso (Pivot center)
    drawBox(4, 6, 2,  0, 0, 0,   0, 0, 0,   0, 0, 0, '#');
    // Head 
    drawBox(2.5, 2.5, 2.5,  0, 0, 0,   0, 4.5, 0,   0, 0, 0, '@');
    // Arms (Pivot at shoulder)
    drawBox(1.5, 5, 1.5,  0, -2.5, 0,  -3.5, 2.5, 0,  currentLimbAngle, 0, 0, '=');
    drawBox(1.5, 5, 1.5,  0, -2.5, 0,   3.5, 2.5, 0, -currentLimbAngle, 0, 0, '=');
    // Legs (Pivot at hips)
    drawBox(1.5, 6, 1.5,  0, -3.0, 0,  -1.5, -3, 0,  -currentLimbAngle, 0, 0, ':');
    drawBox(1.5, 6, 1.5,  0, -3.0, 0,   1.5, -3, 0,   currentLimbAngle, 0, 0, ':');

    printf("\x1b[H");
    for (int k = 0; k < width * height; k++) {
      putchar(k % width ? buffer[k] : 10);
    }
    
    printf("\n  [W/A/S/D] Move Robot   [I/J/K/L] Orbit Camera   [+/-] Zoom   [Esc] Quit\n");

    usleep(20000); // 50fps
  }

  return 0;
}
