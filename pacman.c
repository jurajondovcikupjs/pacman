#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <windows.h>
#include <conio.h>

#define MAP_HEIGHT 31
#define MAP_WIDTH 28

#define WALL '#'
#define EMPTY ' '
#define DOT '.'
#define BIG_DOT 'O'
#define PACMAN 'P'
#define GHOST 'G'

struct Pacman {
    int x;
    int y;
    int lives;
    char texture;
};

struct Ghost {
    int x;
    int y;
    int isChasing;
};

struct PowerUp {
    int x;
    int y;
};

struct Game {
    struct Pacman pacman;
    struct Ghost ghosts[4];
    int score;
    struct PowerUp powerUps[4];
    char map[MAP_HEIGHT][MAP_WIDTH];
};

void clearTerminal() {
    system("cls");
}

void setupConsole(int width, int height) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    SMALL_RECT minRect = { 0, 0, 1, 1 };
    SetConsoleWindowInfo(hOut, TRUE, &minRect);

    COORD bufferSize = { (SHORT)width, (SHORT)height };
    SetConsoleScreenBufferSize(hOut, bufferSize);

    SMALL_RECT windowRect = { 0, 0, (SHORT)(width - 1), (SHORT)(height - 1) };
    SetConsoleWindowInfo(hOut, TRUE, &windowRect);
}

void setCursor(int x, int y) {
    COORD c = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void setCursorVisibility(bool visible) {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = visible ? TRUE : FALSE;
    SetConsoleCursorInfo(consoleHandle, &info);
}

char readMove() {
    if (_kbhit()) {
        int c = _getch();
        if (c == 0 || c == 0xE0) {
            c = _getch();
            switch (c) {
                case 72: return 'w';
                case 80: return 's';
                case 75: return 'a';
                case 77: return 'd';
            }
            return 0;
        }
        return (char)c;
    }
    return 0;
}

void movePacman(struct Game* game, char m) {
    int nextX = game->pacman.x;
    int nextY = game->pacman.y;

    if (m == 'w') nextY--;
    else if (m == 's') nextY++;
    else if (m == 'a') nextX--;
    else if (m == 'd') nextX++;
    else return;

    if (nextY >= 0 && nextY < MAP_HEIGHT && nextX >= 0 && nextX < MAP_WIDTH) {
        char targetTile = game->map[nextY][nextX];
        if (targetTile != WALL) {
            game->pacman.x = nextX;
            game->pacman.y = nextY;

            if (targetTile == DOT) {
                game->score += 10;
                game->map[nextY][nextX] = EMPTY;
            } else if (targetTile == BIG_DOT) {
                game->score += 50;
                game->map[nextY][nextX] = EMPTY;
            }
        }
    }
}

void constructMap(struct Game* game) {
    const char* map[MAP_HEIGHT] = {
        "############################",
        "#............##............#",
        "#.####.#####.##.#####.####.#",
        "#O####.#####.##.#####.####O#",
        "#.####.#####.##.#####.####.#",
        "#..........................#",
        "#.####.##.########.##.####.#",
        "#.####.##.########.##.####.#",
        "#......##....##....##......#",
        "######.##### ## #####.######",
        "     #.##### ## #####.#     ",
        "     #.#            #.#     ",
        "     #.#            #.#     ",
        "     #.# ########## #.#     ",
        "     #.# ########## #.#     ",
        "     #.# ########## #.#     ",
        "     #.# ########## #.#     ",
        "     #.# ########## #.#     ",
        "     #.#            #.#     ",
        "######.## ######## ##.######",
        "#............##............#",
        "#.####.#####.##.#####.####.#",
        "#.####.#####.##.#####.####.#",
        "#O..##.......  .......##..O#",
        "###.##.##.########.##.##.###",
        "###.##.##.########.##.##.###",
        "#......##....##....##......#",
        "#.##########.##.##########.#",
        "#.##########.##.##########.#",
        "#..........................#",
        "############################"
    };

    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            game->map[i][j] = map[i][j];
        }
    }
}

void startNewGame(struct Game* game) {
    game->score = 0;
    game->pacman = (struct Pacman){ 13, 23, 3, PACMAN };
    for (int i = 0; i < 4; i++) {
        game->ghosts[i] = (struct Ghost){ 0 };
        game->powerUps[i] = (struct PowerUp){ 0 };
    }
}

void printMap(struct Game* game) {
    setCursor(0, 0);
    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            if (i == game->pacman.y && j == game->pacman.x) {
                printf("%c", game->pacman.texture);
            } else {
                printf("%c", game->map[i][j]);
            }
        }
        printf("\n");
    }
    printf("Score: %d   Lives: %d", game->score, game->pacman.lives);
    fflush(stdout);
}

int main() {
    struct Game g = { 0 };
    constructMap(&g);
    startNewGame(&g);
    setupConsole(40, MAP_HEIGHT + 4);
    clearTerminal();
    setCursorVisibility(false);
    printMap(&g);

    while (1) {
        char move = readMove();
        if (move == 'q' || move == 'Q') {
            break;
        }

        if (move != 0) {
            int oldX = g.pacman.x;
            int oldY = g.pacman.y;

            movePacman(&g, move);

            if (oldX != g.pacman.x || oldY != g.pacman.y) {
                setCursor(oldX, oldY);
                printf("%c", g.map[oldY][oldX]);

                setCursor(g.pacman.x, g.pacman.y);
                printf("%c", g.pacman.texture);

                setCursor(0, MAP_HEIGHT);
                printf("Score: %d   Lives: %d   ", g.score, g.pacman.lives);

                fflush(stdout);
            }
        }

        Sleep(50);
    }

    setCursorVisibility(true);
    setCursor(0, MAP_HEIGHT + 2);
    return 0;
}