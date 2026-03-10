#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/select.h>

#include "snake_common.h"



static struct termios orig_term;

void platform_cleanup(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
    printf("\033[?25h");
    fflush(stdout);
}

void platform_init(void) {
    tcgetattr(STDIN_FILENO, &orig_term);
    atexit(platform_cleanup);

    struct termios raw = orig_term;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    printf("\033[?25l");
    fflush(stdout);
}

void move_cursor(int x, int y) {
    printf("\033[%d;%dH", y + 1, x + 1);
}

int read_key(void) {
    struct timeval tv = {0, 0};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    if (select(1, &fds, NULL, NULL, &tv) <= 0) return 0;

    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return 0;
    if (c != 0x1b) return (int)c;

    tv.tv_sec = 0; tv.tv_usec = 10000;
    FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
    if (select(1, &fds, NULL, NULL, &tv) <= 0) return 0x1b;

    unsigned char seq[2];
    if (read(STDIN_FILENO, &seq[0], 1) != 1) return 0x1b;
    if (seq[0] != '[') return 0x1b;

    tv.tv_sec = 0; tv.tv_usec = 10000;
    FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
    if (select(1, &fds, NULL, NULL, &tv) <= 0) return 0x1b;

    if (read(STDIN_FILENO, &seq[1], 1) != 1) return 0x1b;

    switch (seq[1]) {
        case 'A': return KEY_UP;
        case 'B': return KEY_DOWN;
        case 'C': return KEY_RIGHT;
        case 'D': return KEY_LEFT;
        default:  return 0;
    }
}

int main(void) {
    run_game();
    return 0;
}
