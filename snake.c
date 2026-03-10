#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>

#define WIDTH 30
#define HEIGHT 20
#define MAX_SNAKE 200

int snakeX[MAX_SNAKE];
int snakeY[MAX_SNAKE];
int length = 3;

int foodX, foodY;

int dirX = 1;
int dirY = 0;

int gameOver = 0;

struct termios orig;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig);
    atexit(disableRawMode);

    struct termios raw = orig;
    raw.c_lflag &= ~(ECHO | ICANON);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void spawnFood() {
    foodX = rand() % WIDTH;
    foodY = rand() % HEIGHT;
}

int kbhit() {
    struct timeval tv = {0L, 0L};
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

void input() {
    if (!kbhit()) return;

    char c = getchar();

    if (c == 'w' && dirY == 0) { dirX = 0; dirY = -1; }
    if (c == 's' && dirY == 0) { dirX = 0; dirY = 1; }
    if (c == 'a' && dirX == 0) { dirX = -1; dirY = 0; }
    if (c == 'd' && dirX == 0) { dirX = 1; dirY = 0; }

    if (c == 'q') gameOver = 1;
}

void update() {

    for (int i = length; i > 0; i--) {
        snakeX[i] = snakeX[i-1];
        snakeY[i] = snakeY[i-1];
    }

    snakeX[0] += dirX;
    snakeY[0] += dirY;

    if (snakeX[0] < 0 || snakeX[0] >= WIDTH ||
        snakeY[0] < 0 || snakeY[0] >= HEIGHT) {
        gameOver = 1;
        }

        for (int i = 1; i < length; i++) {
            if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
                gameOver = 1;
            }
        }

        if (snakeX[0] == foodX && snakeY[0] == foodY) {
            length++;
            spawnFood();
        }
}

void draw() {
    printf("\033[H\033[J");

    for (int y = -1; y <= HEIGHT; y++) {
        for (int x = -1; x <= WIDTH; x++) {

            if (x == -1 || x == WIDTH || y == -1 || y == HEIGHT) {
                printf("#");
                continue;
            }

            if (x == foodX && y == foodY) {
                printf("*");
                continue;
            }

            int printed = 0;

            for (int i = 0; i < length; i++) {
                if (snakeX[i] == x && snakeY[i] == y) {
                    printf(i == 0 ? "O" : "o");
                    printed = 1;
                    break;
                }
            }

            if (!printed) printf(" ");
        }
        printf("\n");
    }

    printf("Score: %d\n", length - 3);
}

int main() {

    srand(time(NULL));
    enableRawMode();

    snakeX[0] = WIDTH / 2;
    snakeY[0] = HEIGHT / 2;

    for (int i = 1; i < length; i++) {
        snakeX[i] = snakeX[0] - i;
        snakeY[i] = snakeY[0];
    }

    spawnFood();

    while (!gameOver) {
        input();
        update();
        draw();
        usleep(120000);
    }

    printf("Game Over! Final score: %d\n", length - 3);

    return 0;
}
