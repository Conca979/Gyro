#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

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

  // Scale down to 90% of the window size to prevent edge-wrapping and flickering
  *w = (int)(*w * 0.95f);
  *h = (int)(*h * 0.95f);
}

float A = 0, B = 0, C = 0;

float cubeWidth = 20;
int width = 0, height = 0;
float *zBuffer = NULL;
char *buffer = NULL;
char *renderBuffer = NULL;
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
#ifdef _WIN32
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD dwMode = 0;
  GetConsoleMode(hOut, &dwMode);
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(hOut, dwMode);
#endif
  setvbuf(stdout, NULL, _IONBF, 0);

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

    int new_w, new_h;
    getTerminalSize(&new_w, &new_h);
    if (new_w != width || new_h != height) {
      width = new_w;
      height = new_h;
      zBuffer = (float *)realloc(zBuffer, width * height * sizeof(float));
      buffer = (char *)realloc(buffer, width * height * sizeof(char));
      renderBuffer = (char *)realloc(renderBuffer, 3 + height * (width + 1) + 256);
      K1 = height * 0.9f;
      printf("\x1b[2J");
    }

    memset(buffer, backgroundASCIICode, width * height);
    memset(zBuffer, 0, width * height * sizeof(float));
    
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
    
    int p = 0;
    renderBuffer[p++] = '\x1b';
    renderBuffer[p++] = '[';
    renderBuffer[p++] = 'H';
    
    for (int y = 0; y < height; y++) {
      memcpy(&renderBuffer[p], &buffer[y * width], width);
      p += width;
      renderBuffer[p++] = '\n';
    }
    
    const char *hud = "  [W/S] Pitch   [A/D] Yaw   [Q/E] Roll   [+/-] Zoom   [Space] Pause   [Esc] Quit\n";
    int hud_len = strlen(hud);
    memcpy(&renderBuffer[p], hud, hud_len);
    p += hud_len;

    fwrite(renderBuffer, 1, p, stdout);
    fflush(stdout);

    usleep(8000 * 2);
  }
  
  return 0;
}
