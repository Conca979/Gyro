#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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

float A = 0, B = 0, C = 0;

float cubeWidth = 20;
int width = 160, height = 44;
float zBuffer[160 * 44];
char buffer[160 * 44];
int backgroundASCIICode = ' ';
int distanceFromCam = 100;
float horizontalOffset;
float K1 = 40;

float incrementSpeed = 0.6;

float x, y, z;
float ooz;
int xp, yp;
int idx;

float calculateX(int i, int j, int k) {
  return (
    j * sin(A) * sin(B) * cos(C) - k * cos(A) * sin(B) * cos(C) + 
    j * cos(A) * sin(C) + k * sin(A) * sin(C) + i * cos(B) * cos(C)
  );
}

float calculateY(int i, int j, int k) {
  return (
    j * cos(A) * cos(C) + k * sin(A) * cos(C) - 
    j * sin(A) * sin(B) * sin(C) + k * cos(A) * sin(B) * sin(C) - 
    i * cos(B) * sin(C)
  );
}

float calculateZ(int i, int j, int k) {
  return (
    k * cos(A) * cos(B) - 
    j * sin(A) * cos(B) + i * sin(B)
  );
}

void calculateForSurface(float cubeX, float cubeY, float cubeZ, int ch) {
  x = calculateX(cubeX, cubeY, cubeZ);
  y = calculateY(cubeX, cubeY, cubeZ);
  z = calculateZ(cubeX, cubeY, cubeZ) + distanceFromCam;

  if (z <= 0.1f) return;

  ooz = 1 / z;

  xp = (int)(width / 2 + horizontalOffset + K1 * ooz * x * 2);
  yp = (int)(height / 2 + K1 * ooz * y);

  idx = xp + yp * width;
  if (idx >= 0 && idx < width * height) {
    if (ooz > zBuffer[idx]) {
      zBuffer[idx] = ooz;
      buffer[idx] = ch;
    }
  }
}

int main() {
  int paused = 0;
  printf("\x1b[2J");
  while (1) {
    if (_kbhit()) {
      char key = _getch();
      if (key == 27) break; // Esc
      if (key == 'w' || key == 'W') A -= 0.1f;
      if (key == 's' || key == 'S') A += 0.1f;
      if (key == 'a' || key == 'A') B -= 0.1f;
      if (key == 'd' || key == 'D') B += 0.1f;
      if (key == 'q' || key == 'Q') C -= 0.1f;
      if (key == 'e' || key == 'E') C += 0.1f;
      if (key == '=' || key == '+') distanceFromCam -= 5;
      if (key == '-' || key == '_') distanceFromCam += 5;
      if (key == ' ') paused = !paused;
    }

    if (!paused) {
      A += 0.05;
      B += 0.05;
      C += 0.01;
    }

    memset(buffer, backgroundASCIICode, width * height);
    memset(zBuffer, 0, width * height * 4);
    
    // first cube
    cubeWidth = 20;
    horizontalOffset = -2 * cubeWidth;
    for (float cubeX = -cubeWidth; cubeX < cubeWidth; cubeX += incrementSpeed) {
      for (float cubeY = -cubeWidth; cubeY < cubeWidth; cubeY += incrementSpeed) {
        calculateForSurface(cubeX, cubeY, -cubeWidth, '@');
        calculateForSurface(cubeWidth, cubeY, cubeX, '$');
        calculateForSurface(-cubeWidth, cubeY, -cubeX, '~');
        calculateForSurface(-cubeX, cubeY, cubeWidth, '#');
        calculateForSurface(cubeX, -cubeWidth, -cubeY, ';');
        calculateForSurface(cubeX, cubeWidth, cubeY, '+');
      }
    }
    
    // second cube
    cubeWidth = 10;
    horizontalOffset = 1 * cubeWidth;
    for (float cubeX = -cubeWidth; cubeX < cubeWidth; cubeX += incrementSpeed) {
      for (float cubeY = -cubeWidth; cubeY < cubeWidth; cubeY += incrementSpeed) {
        calculateForSurface(cubeX, cubeY, -cubeWidth, '@');
        calculateForSurface(cubeWidth, cubeY, cubeX, '$');
        calculateForSurface(-cubeWidth, cubeY, -cubeX, '~');
        calculateForSurface(-cubeX, cubeY, cubeWidth, '#');
        calculateForSurface(cubeX, -cubeWidth, -cubeY, ';');
        calculateForSurface(cubeX, cubeWidth, cubeY, '+');
      }
    }
    
    // third cube
    cubeWidth = 5;
    horizontalOffset = 8 * cubeWidth;
    for (float cubeX = -cubeWidth; cubeX < cubeWidth; cubeX += incrementSpeed) {
      for (float cubeY = -cubeWidth; cubeY < cubeWidth; cubeY += incrementSpeed) {
        calculateForSurface(cubeX, cubeY, -cubeWidth, '@');
        calculateForSurface(cubeWidth, cubeY, cubeX, '$');
        calculateForSurface(-cubeWidth, cubeY, -cubeX, '~');
        calculateForSurface(-cubeX, cubeY, cubeWidth, '#');
        calculateForSurface(cubeX, -cubeWidth, -cubeY, ';');
        calculateForSurface(cubeX, cubeWidth, cubeY, '+');
      }
    }
    
    printf("\x1b[H");
    for (int k = 0; k < width * height; k++) {
      putchar(k % width ? buffer[k] : 10);
    }
    
    printf("\n  [W/S] Pitch   [A/D] Yaw   [Q/E] Roll   [+/-] Zoom   [Space] Pause   [Esc] Quit\n");

    usleep(8000 * 2);
  }
  
  return 0;
}
