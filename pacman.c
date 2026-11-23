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

char ghostTextures[4] = { 'B', 'P', 'I', 'C' };

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
    int direction; // 0 = Up, 1 = Right, 2 = Down, 3 = Left
    int prevX;
    int prevY;
};

struct PowerUp {
    int id;
    int x;
    int y;
};

struct Game {
    struct Pacman pacman;
    struct Ghost ghosts[4]; // 0 - Blinky, 1 - Pinky, 2 - Inky, 3 - Clyde
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

int calculateDistance(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

int decideGhostMove(struct Game* game, int ghostIndex, int targetX, int targetY) {
    int gx = game->ghosts[ghostIndex].x;
    int gy = game->ghosts[ghostIndex].y;
    int prevX = game->ghosts[ghostIndex].prevX;
    int prevY = game->ghosts[ghostIndex].prevY;

    int distance = 999999;
    int nextMove = -1;

    // 0: Up
    if (gy > 0 && game->map[gy - 1][gx] != WALL && !(gx == prevX && gy - 1 == prevY)) {
        int upDistance = calculateDistance(gx, gy - 1, targetX, targetY);
        if (upDistance < distance) {
            distance = upDistance;
            nextMove = 0;
        }
    }

    // 2: Down
    if (gy < MAP_HEIGHT - 1 && game->map[gy + 1][gx] != WALL && !(gx == prevX && gy + 1 == prevY)) {
        int downDistance = calculateDistance(gx, gy + 1, targetX, targetY);
        if (downDistance < distance) {
            distance = downDistance;
            nextMove = 2;
        }
    }

    // 3: Left
    if (gx > 0 && game->map[gy][gx - 1] != WALL && !(gx - 1 == prevX && gy == prevY)) {
        int leftDistance = calculateDistance(gx - 1, gy, targetX, targetY);
        if (leftDistance < distance) {
            distance = leftDistance;
            nextMove = 3;
        }
    }

    // 1: Right
    if (gx < MAP_WIDTH - 1 && game->map[gy][gx + 1] != WALL && !(gx + 1 == prevX && gy == prevY)) {
        int rightDistance = calculateDistance(gx + 1, gy, targetX, targetY);
        if (rightDistance < distance) {
            distance = rightDistance;
            nextMove = 1;
        }
    }

    // Fallback: if trapped in a dead end, allow stepping back to prevX/prevY
    if (nextMove == -1) {
        if (prevY < gy) return 0;
        if (prevY > gy) return 2;
        if (prevX < gx) return 3;
        if (prevX > gx) return 1;
    }

    return nextMove;
}

void moveGhostToTarget(struct Game* game, int ghostIndex, int targetX, int targetY) {
    int nextMove = decideGhostMove(game, ghostIndex, targetX, targetY);
    if (nextMove != -1) {
        int nextX = game->ghosts[ghostIndex].x + (nextMove == 1 ? 1 : (nextMove == 3 ? -1 : 0));
        int nextY = game->ghosts[ghostIndex].y + (nextMove == 2 ? 1 : (nextMove == 0 ? -1 : 0));
        if (nextY >= 0 && nextY < MAP_HEIGHT && nextX >= 0 && nextX < MAP_WIDTH) {
            if (game->map[nextY][nextX] != WALL) {
                game->ghosts[ghostIndex].prevX = game->ghosts[ghostIndex].x;
                game->ghosts[ghostIndex].prevY = game->ghosts[ghostIndex].y;
                game->ghosts[ghostIndex].x = nextX;
                game->ghosts[ghostIndex].y = nextY;
            }
        }
    }
}

void moveBlinky(struct Game* game) {
    int targetX = game->pacman.x;
    int targetY = game->pacman.y;
    moveGhostToTarget(game, 0, targetX, targetY);
}

void movePinky(struct Game* game) {
    int targetX = game->pacman.x;
    int targetY = game->pacman.y;

    if (game->pacman.direction == 'w') {
        targetY -= 4;
        // Fun arcade trivia: the original 1980 game had an overflow bug
        // that also shifted targetX -= 4 when facing Up!
    } else if (game->pacman.direction == 's') {
        targetY += 4;
    } else if (game->pacman.direction == 'a') {
        targetX -= 4;
    } else if (game->pacman.direction == 'd') {
        targetX += 4;
    }

    moveGhostToTarget(game, 1, targetX, targetY);
}

void moveInky(struct Game* game) {
    int pivotX = game->pacman.x;
    int pivotY = game->pacman.y;

    if (game->pacman.direction == 'w') pivotY -= 2;
    else if (game->pacman.direction == 's') pivotY += 2;
    else if (game->pacman.direction == 'a') pivotX -= 2;
    else if (game->pacman.direction == 'd') pivotX += 2;

    int targetX = pivotX + (pivotX - game->ghosts[0].x);
    int targetY = pivotY + (pivotY - game->ghosts[0].y);

    moveGhostToTarget(game, 2, targetX, targetY);
}

void moveClyde(struct Game* game) {
    int dist = calculateDistance(game->ghosts[3].x, game->ghosts[3].y, game->pacman.x, game->pacman.y);
    if (dist > 8) {
        moveGhostToTarget(game, 3, game->pacman.x, game->pacman.y);
    } else {
        moveGhostToTarget(game, 3, 0, MAP_HEIGHT - 1);
    }
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
    game->pacman = (struct Pacman){ .x = 13, .y = 23, .lives = 3, .texture = PACMAN, .direction = 0};
    for (int i = 0; i < 4; i++) {
        game->ghosts[i] = (struct Ghost){ .id = i, .x = 12 + i, .y = 12, .isChasing = 1, .texture = ghostTextures[i], .direction = 0, .prevX = -1, .prevY = -1  };
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

char getTileAt(struct Game* game, int x, int y) {
    if (game->pacman.x == x && game->pacman.y == y) return game->pacman.texture;
    for (int i = 0; i < 4; i++) {
        if (game->ghosts[i].x == x && game->ghosts[i].y == y) return game->ghosts[i].texture;
    }
    return game->map[y][x];
}

bool checkGhostCollision(struct Game* game) {
    for (int i = 0; i < 4; i++) {
        if (game->ghosts[i].x == game->pacman.x && game->ghosts[i].y == game->pacman.y) {
            return true;
        }
    }
    return false;
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

            int oldPacmanX = g.pacman.x;
            int oldPacmanY = g.pacman.y;

            int oldBlinkyX = g.ghosts[0].x;
            int oldBlinkyY = g.ghosts[0].y;

            int oldPinkyX = g.ghosts[1].x;
            int oldPinkyY = g.ghosts[1].y;

            int oldInkyX = g.ghosts[2].x;
            int oldInkyY = g.ghosts[2].y;

            int oldClydeX = g.ghosts[3].x;
            int oldClydeY = g.ghosts[3].y;

            int oldScore = g.score;
            char oldDirection = g.pacman.direction;

            //now we use the cached wanted direction to move pacman
            movePacman(&g, g.pacman.wantedDirection);
            if (oldPacmanX == g.pacman.x && oldPacmanY == g.pacman.y) {
                movePacman(&g, g.pacman.direction);
            } else {
                g.pacman.direction = g.pacman.wantedDirection;
            }

            // update Blinky
            moveBlinky(&g);
            if (oldBlinkyX != g.ghosts[0].x || oldBlinkyY != g.ghosts[0].y) {
                setCursor(oldBlinkyX, oldBlinkyY);
                printf("%c", getTileAt(&g, oldBlinkyX, oldBlinkyY));

                setCursor(g.ghosts[0].x, g.ghosts[0].y);
                printf("%c", g.ghosts[0].texture);

                fflush(stdout);
            }

            // update Pinky
            movePinky(&g);
            if (oldPinkyX != g.ghosts[1].x || oldPinkyY != g.ghosts[1].y) {
                setCursor(oldPinkyX, oldPinkyY);
                printf("%c", getTileAt(&g, oldPinkyX, oldPinkyY));

                setCursor(g.ghosts[1].x, g.ghosts[1].y);
                printf("%c", g.ghosts[1].texture);

                fflush(stdout);
            }

            //update Inky
            moveInky(&g);
            if (oldInkyX != g.ghosts[2].x || oldInkyY != g.ghosts[2].y) {
                setCursor(oldInkyX, oldInkyY);
                printf("%c", getTileAt(&g, oldInkyX, oldInkyY));

                setCursor(g.ghosts[2].x, g.ghosts[2].y);
                printf("%c", g.ghosts[2].texture);

                fflush(stdout);
            }

            //update Clyde
            moveClyde(&g);
            if (oldClydeX != g.ghosts[3].x || oldClydeY != g.ghosts[3].y) {
                setCursor(oldClydeX, oldClydeY);
                printf("%c", getTileAt(&g, oldClydeX, oldClydeY));

                setCursor(g.ghosts[3].x, g.ghosts[3].y);
                printf("%c", g.ghosts[3].texture);

                fflush(stdout);
            }

            //update the cursor if pacman moved
            if (oldPacmanX != g.pacman.x || oldPacmanY != g.pacman.y) {
                setCursor(oldPacmanX, oldPacmanY);
                printf("%c", g.map[oldPacmanY][oldPacmanX]);

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
