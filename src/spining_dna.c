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

float angleX = 0.3f;
float angleY = 0.0f;
float angleZ = 0.0f;
float camDist = 25.0f;
float K1 = 40.0f;

float cosX, sinX, cosY, sinY, cosZ, sinZ;

void drawPoint(float x, float y, float z, char ch) {
  // Rotate X
  float ty = y * cosX - z * sinX;
  float tz = y * sinX + z * cosX;
  y = ty; z = tz;
  
  // Rotate Y
  float tx = x * cosY + z * sinY;
  tz = -x * sinY + z * cosY;
  x = tx; z = tz;
  
  // Rotate Z
  tx = x * cosZ - y * sinZ;
  ty = x * sinZ + y * cosZ;
  x = tx; y = ty;
  
  z += camDist;
  if (z <= 0.1f) return; 
  
  float ooz = 1.0f / z;
  int xp = (int)(width / 2.0f + K1 * ooz * x * 2.0f);
  int yp = (int)(height / 2.0f + K1 * ooz * y);
  
  int idx = xp + yp * width;
  if (xp >= 0 && xp < width && yp >= 0 && yp < height) {
    if (ooz > zBuffer[idx]) {
      zBuffer[idx] = ooz;
      buffer[idx] = ch;
    }
  }
}

void drawLine(float x1, float y1, float z1, float x2, float y2, float z2, char ch) {
  int steps = 15;
  for (int i = 0; i <= steps; i++) {
    float t = (float)i / steps;
    float x = x1 + (x2 - x1) * t;
    float y = y1 + (y2 - y1) * t;
    float z = z1 + (z2 - z1) * t;
    drawPoint(x, y, z, ch);
  }
}

int main() {
  int paused = 0;
  float L = 24.0f;     // Total length of the DNA
  float R = 4.0f;      // Radius of the helix
  float turnRate = 0.6f; // How tight the helix is wound
  float PI = 3.14159f;

  printf("\x1b[2J");

  while (1) {
    if (_kbhit()) {
      char key = _getch();
      if (key == 27) break; // Esc
      if (key == 'w' || key == 'W') angleX -= 0.15f;
      if (key == 's' || key == 'S') angleX += 0.15f;
      if (key == 'a' || key == 'A') angleY -= 0.15f;
      if (key == 'd' || key == 'D') angleY += 0.15f;
      if (key == 'q' || key == 'Q') angleZ -= 0.15f;
      if (key == 'e' || key == 'E') angleZ += 0.15f;
      if (key == '=' || key == '+') camDist -= 2.0f;
      if (key == '-' || key == '_') camDist += 2.0f;
      if (key == ' ') paused = !paused;
    }

    if (!paused) {
      angleY += 0.05f; // Auto rotate
    }

    memset(buffer, ' ', width * height);
    memset(zBuffer, 0, width * height * sizeof(float));

    cosX = cos(angleX); sinX = sin(angleX);
    cosY = cos(angleY); sinY = sin(angleY);
    cosZ = cos(angleZ); sinZ = sin(angleZ);

    // Draw the two helix strands
    for (float y = -L/2; y <= L/2; y += 0.05f) {
      float theta = y * turnRate;
      drawPoint(R * cos(theta), y, R * sin(theta), '@');
      drawPoint(R * cos(theta + PI), y, R * sin(theta + PI), '#');
    }

    // Draw the connecting rungs (base pairs)
    for (float y = -L/2 + 1.0f; y <= L/2 - 1.0f; y += 1.5f) {
      float theta = y * turnRate;
      float x1 = R * cos(theta);
      float z1 = R * sin(theta);
      float x2 = R * cos(theta + PI);
      float z2 = R * sin(theta + PI);
      drawLine(x1, y, z1, x2, y, z2, '=');
    }

    printf("\x1b[H");
    for (int k = 0; k < width * height; k++) {
      putchar(k % width ? buffer[k] : 10);
    }
    
    printf("\n  [W/S] Pitch   [A/D] Yaw   [Q/E] Roll   [+/-] Zoom   [Space] Pause   [Esc] Quit\n");

    usleep(20000); // 20ms = ~50fps
  }

  return 0;
}
