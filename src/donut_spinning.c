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

  *w = (int)(*w * 0.95f);
  *h = (int)(*h * 0.95f);
}

int width = 0;
int height = 0;

float A = 0, B = 0;
float *zBuffer = NULL;
char *buffer = NULL;
char *renderBuffer = NULL;
float camDist = 5.0f; // K2 distance

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
      if (key == 'w' || key == 'W') A -= 0.15f;
      if (key == 's' || key == 'S') A += 0.15f;
      if (key == 'a' || key == 'A') B -= 0.15f;
      if (key == 'd' || key == 'D') B += 0.15f;
      if (key == '=' || key == '+') camDist -= 0.5f;
      if (key == '-' || key == '_') camDist += 0.5f;
      if (key == ' ') paused = !paused;
    }

    if (!paused) {
      A += 0.04;
      B += 0.02;
    }

    int new_w, new_h;
    getTerminalSize(&new_w, &new_h);
    if (new_w != width || new_h != height) {
      width = new_w;
      height = new_h;
      zBuffer = (float *)realloc(zBuffer, width * height * sizeof(float));
      buffer = (char *)realloc(buffer, width * height * sizeof(char));
      renderBuffer = (char *)realloc(renderBuffer, 3 + height * (width + 1) + 256);
      printf("\x1b[2J");
    }

    memset(buffer, ' ', width * height);
    memset(zBuffer, 0, width * height * sizeof(float));

    float cosA = cos(A), sinA = sin(A);
    float cosB = cos(B), sinB = sin(B);

    for (float theta = 0; theta < 6.28; theta += 0.03) {
      float costheta = cos(theta), sintheta = sin(theta);

      for (float phi = 0; phi < 6.28; phi += 0.01) {
        float cosphi = cos(phi), sinphi = sin(phi);

        float h = costheta + 2; 
        float D = 1 / (sinphi * h * sinA + sintheta * cosA + camDist); 
        float t = sinphi * h * cosA - sintheta * sinA;

        float K1 = height * 0.68f;
        int xp = (int)(width / 2 + (K1 * 2.0f) * D * (cosphi * h * cosB - t * sinB));
        int yp = (int)(height / 2 + K1 * D * (cosphi * h * sinB + t * cosB));

        int o = xp + width * yp;

        int N = (int)(8 * ((sintheta * sinA - sinphi * costheta * cosA) * cosB - sinphi * costheta * sinA - sintheta * cosA - cosphi * costheta * sinB));

        if (height > yp && yp >= 0 && xp >= 0 && width > xp && D > zBuffer[o]) {
          zBuffer[o] = D;
          int luminance = N > 0 ? N : 0;
          if (luminance > 11) luminance = 11;
          buffer[o] = ".,-~:;=!*#$@"[luminance];
        }
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
    
    const char *hud = "  [W/S] Pitch   [A/D] Yaw   [+/-] Zoom   [Space] Pause   [Esc] Quit\n";
    int hud_len = strlen(hud);
    memcpy(&renderBuffer[p], hud, hud_len);
    p += hud_len;

    fwrite(renderBuffer, 1, p, stdout);
    fflush(stdout);

    usleep(20000); 
  }
  return 0;
}