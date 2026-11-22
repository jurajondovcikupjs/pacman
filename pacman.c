#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <synchapi.h>
#include <windows.h>
#include <conio.h>

#define MAP_HEIGHT 31
#define MAP_WIDTH 28

#define PACMAN_MOVE_DELAY 4

#define WALL '#'
#define EMPTY ' '
#define DOT '.'
#define BIG_DOT 'O'
#define PACMAN 'P'
#define GHOST 'G'

int level = 1;

struct Pacman {
    int x;
    int y;
    int lives;
    char texture;
    char direction;
    char wantedDirection;
    int isChasing;
};


struct Ghost {
    int id;
    int x;
    int y;
    int isChasing;
    char texture;
};

struct PowerUp {
    int id;
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

//clears the terminal
void clearTerminal() {
    system("cls");
}

//sets up the console window size and title, optimization
void setupConsole(int width, int height) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    SMALL_RECT minRect = { 0, 0, 1, 1 };
    SetConsoleWindowInfo(hOut, TRUE, &minRect);

    COORD bufferSize = { (SHORT)width, (SHORT)height };
    SetConsoleScreenBufferSize(hOut, bufferSize);

    SMALL_RECT windowRect = { 0, 0, (SHORT)(width - 1), (SHORT)(height - 1) };
    SetConsoleWindowInfo(hOut, TRUE, &windowRect);

    SetConsoleTitle("PaCman");
}

//sets the cursor position to the specified x and y coordinates, so no need to clear terminal every time
void setCursor(int x, int y) {
    COORD c = { (SHORT)x, (SHORT)y };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

//hides the cursor
void setCursorVisibility(bool visible) {
    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO info;
    info.dwSize = 100;
    info.bVisible = visible ? TRUE : FALSE;
    SetConsoleCursorInfo(consoleHandle, &info);
}

//just a logo - splash screen
void showLogo(struct Game* game) {
    const char* pacman_art[31] = {
        "        .########.          ",
        "      .############.        ",
        "     ################       ",
        "    ##################      ",
        "   #############..#####     ",
        "  #############....#####    ",
        "  #############....#####    ",
        " ###############..#######   ",
        " ####################       ",
        " #################          ",
        " ##############             ",
        " ###########                ",
        " ########         O    O    ",
        " ########                   ",
        " ########                   ",
        " ########         O    O    ",
        " ###########                ",
        " ##############             ",
        " #################          ",
        " ####################       ",
        " ########################   ",
        "  ######################    ",
        "  ######################    ",
        "   ####################     ",
        "    ##################      ",
        "     ################       ",
        "      '############'        ",
        "        '########'          ",
        "                            ",
        "   P a C m a n   2 8 x 31   ",
        "       Juraj Ondovcik       "
    };

    for (int i = 0; i < 31; i++) {
        printf("%s\n", pacman_art[i]);
    }
    Sleep(2000);
    clearTerminal();
}

//reads the user's move from the keyboard, either aswd or arrow keys
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

//changes the facing direction of the pacman based on the move character
void changeFacing(struct Pacman* pacman, char m) {
    if (m == 'w') pacman->direction = 0;
    else if (m == 's') pacman->direction = 2;
    else if (m == 'a') pacman->direction = 3;
    else if (m == 'd') pacman->direction = 1;
}

//moves the pacman based on the move character
void movePacman(struct Game* game, char m) {
    //next position
    int nextX = game->pacman.x;
    int nextY = game->pacman.y;

    //checking if its possible to make the move
    if (m == 'w' && nextY > 0 && game->map[nextY - 1][nextX] != WALL) nextY--;
    else if (m == 's' && nextY < MAP_HEIGHT - 1 && game->map[nextY + 1][nextX] != WALL) nextY++;
    else if (m == 'a' && nextX > 0 && game->map[nextY][nextX - 1] != WALL) nextX--;
    else if (m == 'd' && nextX < MAP_WIDTH - 1 && game->map[nextY][nextX + 1] != WALL) nextX++;
    else return;

    //updating pacman's position if the move is valid
    if (nextY >= 0 && nextY < MAP_HEIGHT && nextX >= 0 && nextX < MAP_WIDTH) {
        char targetTile = game->map[nextY][nextX];
        if (targetTile != WALL) {
            game->pacman.x = nextX;
            game->pacman.y = nextY;

            //updating score and map based on the target tile and leaving the tile empty
            if (targetTile == DOT) {
                game->score += (10 * level);
                game->map[nextY][nextX] = EMPTY;
            } else if (targetTile == BIG_DOT) {
                game->score += (50 * level);
                game->map[nextY][nextX] = EMPTY;
            }
        }
    }
}

void moveBlinky(struct Game* game) {

}

void movePinky(struct Game* game) {

}

void moveInky(struct Game* game) {

}

void moveClyde(struct Game* game) {

}

//constructs the map for the game
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

//starts a new game by resetting the score and pacman/ghost positions
void startNewGame(struct Game* game) {
    game->score = 0;
    game->pacman = (struct Pacman){ .x = 13, .y = 23, .lives = 3, .texture = PACMAN };
    for (int i = 0; i < 4; i++) {
        game->ghosts[i] = (struct Ghost){ .id = i, .x = 12 + i, .y = 12, .isChasing = 1, .texture = GHOST };
    }
}

//prints the map to the console with the stats
void printMap(struct Game* game) {
    setCursor(0, 0);
    for (int i = 0; i < MAP_HEIGHT; i++) {
        for (int j = 0; j < MAP_WIDTH; j++) {
            if (i == game->pacman.y && j == game->pacman.x) {
                printf("%c", game->pacman.texture);
            } else if (i == game->ghosts[0].y && j == game->ghosts[0].x) {
                printf("%c", game->ghosts[0].texture);
            } else if (i == game->ghosts[1].y && j == game->ghosts[1].x) {
                printf("%c", game->ghosts[1].texture);
            } else if (i == game->ghosts[2].y && j == game->ghosts[2].x) {
                printf("%c", game->ghosts[2].texture);
            } else if (i == game->ghosts[3].y && j == game->ghosts[3].x) {
                printf("%c", game->ghosts[3].texture);
            } else {
                printf("%c", game->map[i][j]);
            }
        }
        printf("\n");
    }
    printf("Lives: %d   Score: %d", game->pacman.lives, game->score);
    fflush(stdout);
}
int main() {
    struct Game g = { 0 };
    constructMap(&g);
    startNewGame(&g);
    setupConsole(40, MAP_HEIGHT + 4);
    clearTerminal();
    showLogo(&g);
    setCursorVisibility(false);
    printMap(&g);

    int moveTimer = 0;

    while (1) {
        char move = readMove();

        //leave the game
        if (move == 'q' || move == 'Q') {
            break;
        }

        //cache wanted direction
        if (move == 'w' || move == 'a' || move == 's' || move == 'd') {
            g.pacman.wantedDirection = move;
        }


        //update pacman's direction if the move timer has elapsed
        if (++moveTimer >= PACMAN_MOVE_DELAY) {
            moveTimer = 0;

            int oldX = g.pacman.x;
            int oldY = g.pacman.y;
            int oldScore = g.score;
            char oldDirection = g.pacman.direction;

            //now we use the cached wanted direction to move pacman
            movePacman(&g, g.pacman.wantedDirection);
            if (oldX == g.pacman.x && oldY == g.pacman.y) {
                movePacman(&g, g.pacman.direction);
            } else {
                g.pacman.direction = g.pacman.wantedDirection;
            }


            //update the cursor if pacman moved
            if (oldX != g.pacman.x || oldY != g.pacman.y) {
                setCursor(oldX, oldY);
                printf("%c", g.map[oldY][oldX]);

                setCursor(g.pacman.x, g.pacman.y);
                printf("%c", g.pacman.texture);

                setCursor(0, MAP_HEIGHT);
                printf("Score: %d   Lives: %d   ", g.score, g.pacman.lives); //stats

                fflush(stdout);
            }
        }

        Sleep(50);
    }
}
