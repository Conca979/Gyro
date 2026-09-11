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

const int width = 160;
const int height = 44;

float A = 0, B = 0;
float zBuffer[160 * 44];
char buffer[160 * 44];
float camDist = 5.0f; // K2 distance

int main() {
  int paused = 0;
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

        int xp = (int)(width / 2 + 60 * D * (cosphi * h * cosB - t * sinB));
        int yp = (int)(height / 2 + 30 * D * (cosphi * h * sinB + t * cosB));

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

    printf("\x1b[H");
    for (int k = 0; k < width * height; k++) {
      putchar(k % width ? buffer[k] : 10);
    }
    
    printf("\n  [W/S] Pitch   [A/D] Yaw   [+/-] Zoom   [Space] Pause   [Esc] Quit\n");

    usleep(20000); 
  }
  return 0;
}