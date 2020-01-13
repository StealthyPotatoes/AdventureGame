#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>
#include <time.h>

#define TEXTHEIGHT (int)10
#define TEXTWIDTH (int)6
#define BILLION 1E9
#define SCREENWIDTH 1000
#define SCREENHEIGHT 800


static const char *event_names[] = {        //List copied from some site, lost link :/
        "",
        "",
        "KeyPress",
        "KeyRelease",
        "ButtonPress",
        "ButtonRelease",
        "MotionNotify",
        "EnterNotify",
        "LeaveNotify",
        "FocusIn",
        "FocusOut",
        "KeymapNotify",
        "Expose",
        "GraphicsExpose",
        "NoExpose",
        "VisibilityNotify",
        "CreateNotify",
        "DestroyNotify",
        "UnmapNotify",
        "MapNotify",
        "MapRequest",
        "ReparentNotify",
        "ConfigureNotify",
        "ConfigureRequest",
        "GravityNotify",
        "ResizeRequest",
        "CirculateNotify",
        "CirculateRequest",
        "PropertyNotify",
        "SelectionClear",
        "SelectionRequest",
        "SelectionNotify",
        "ColormapNotify",
        "ClientMessage",
        "MappingNotify"
};

const char title[][100] = {       // ASCII Gen = http://patorjk.com/software/taag/
        "  __ _  ___ _ __   ___ _ __(_) ___    __ _  __ _ _ __ ___   ___",
        " / _` |/ _ \\ '_ \\ / _ \\ '__| |/ __|  / _` |/ _` | '_ ` _ \\ / _ \\",
        "| (_| |  __/ | | |  __/ |  | | (__  | (_| | (_| | | | | | |  __/",
        " \\__, |\\___|_| |_|\\___|_|  |_|\\___|  \\__, |\\__,_|_| |_| |_|\\___|",
        " |___/                               |___/"
};

/*
struct BulletList{
    int x, y;
    char keys[3];
    struct BulletList *next;
};
*/

typedef struct BulletList {
    int x, y;
    char keys[3];
    struct BulletList *next;
} *bulletNode;

typedef struct ExplosionList {
    int x, y, xVel[5], yVel[5], direction, counter;
    struct ExplosionList *next;
} *explosionNode;

struct Button {
    int x, y, width, height;
};

struct MenuButtons {
    struct Button start, editor;
};

struct Box {
    int x, y, type, gc;     //type 1 = door, 2 = key
    char character;
};

struct Level {
    int gc, type;
    char character;
    Bool draw;
};

const struct Box player[7] = {{6,  10, 0, 0, 79},
                             {6,  20, 0, 0, 124},
                             {6,  30, 0, 0, 124},
                             {12, 40, 0, 0, 92},
                             {12, 20, 0, 0, 92},
                             {0,  40, 0, 0, 47},
                             {0,  20, 0, 0, 47}};

struct MenuButtons buttons;
Display *d;
Window w;
GC gc[4];
XColor xc_grey, xc_red, xc_green;
Colormap c;
int oldPlayerX = 0;
int oldPlayerY = 0;
int part = 0;        // The part of the level being edited
int drawKey = 1;
int drawDoor = 1;

char keyPressed(XEvent e);
void drawMenu();
void drawLevel(struct Level level[167][80]);
void drawEditor(struct Level level[167][80], int drawType, int colour);
void showMap(struct Level level[35][167][80]);
void showSpawner(struct Level level[167][80]);
void drawPlayer(int x, int y);
int menuLoop();
int gameLoop();
int editorLoop();
Bool checkIfKeysReleased(Display *display, XEvent *e, XPointer arg);
Bool checkIfKeyPress(Display *display, XEvent *e, XPointer keys);
Bool checkIfNotKeyEvent(Display *display, XEvent *e, XPointer arg);
//void explosion(int x, int y, int *counter);
bulletNode createBulletNode();
bulletNode addBullet(bulletNode bullets, int playerX, int playerY, char keys[3]);
bulletNode bulletTick(bulletNode bullets, struct Level level[167][80]);
//explosionNode createExplosionNode();


int main(void) {

    system("xset r off");      //Disable Key Auto repeat - global across client (spooky)

    d = XOpenDisplay(NULL);

    int screen = DefaultScreen(d);
    int black = BlackPixel(d, screen);
    int white = WhitePixel(d, screen);

    w = XCreateSimpleWindow(d, RootWindow(d, screen),
                            10, 10, SCREENWIDTH, SCREENHEIGHT, 0, white, black);

    XSelectInput(d, w,
                 StructureNotifyMask | KeyPressMask | ExposureMask | KeyReleaseMask | ButtonPressMask |
                 ButtonMotionMask);
    XMapWindow(d, w);

    gc[0] = XCreateGC(d, w, 0, 0);
    XSetForeground(d, gc[0], white);

    // Colour stuff
    c = DefaultColormap(d, 0);
    gc[1] = XCreateGC(d, w, 0, 0);
    gc[2] = XCreateGC(d, w, 0, 0);
    gc[3] = XCreateGC(d, w, 0, 0);
    char grey[] = "grey";
    XParseColor(d, c, grey, &xc_grey);
    XAllocColor(d, c, &xc_grey);
    XSetForeground(d, gc[1], xc_grey.pixel);
    char red[] = "red";
    XParseColor(d, c, red, &xc_red);
    XAllocColor(d, c, &xc_red);
    XSetForeground(d, gc[2], xc_red.pixel);
    char green[] = "green";
    XParseColor(d, c, green, &xc_green);
    XAllocColor(d, c, &xc_green);
    XSetForeground(d, gc[3], xc_green.pixel);

    while (1)       //Wait for window to be mapped before starting main loop
    {
        XEvent e;
        XNextEvent(d, &e);
        if (e.type == Expose) {
            drawMenu();
            break;
        }
    }

    int state = 1;

    while (1) {              // Switch between game states (menu, game, level editor)
        switch (state) {
            case 0:
                return 0;
            case 1:
                state = menuLoop(d, w, gc);
                break;
            case 2:
                state = gameLoop(d, w, gc);
                break;
            case 3:
                state = editorLoop(d, w, gc);
                break;
        }
    }
}

bulletNode createBulletNode() {
    bulletNode temp;
    temp = (bulletNode)malloc(sizeof(struct BulletList));
    temp->next = NULL;
    return temp;
}

/*
explosionNode createExplosionNode() {
    explosionNode temp;
    temp = (explosionNode)malloc(sizeof(explosionNode));
    temp->next = NULL;
    return temp;
}
 */

char keyPressed(XEvent e) {      //Returns key pressed

    char letter[1];
    KeySym key = 0;
    XLookupString(&e.xkey, letter, sizeof(char), &key, 0);
//    printf("key = %d\n", *letter);
    return letter[0];
}

void drawMenu() {      //Displays menu, including buttons and text
    XClearWindow(d, w);
    XStoreName(d, w, "what");

    int menuX = 200;
    int menuY = 150;
    XDrawRectangle(d, w, gc[0], menuX, menuY, 600, 500);     //Big box

    for (int i = 0; i < 5; i++) {
        XDrawString(d, w, gc[0], 300, (240 + 10 * i), title[i], (int)strlen(title[i]));
    }

    buttons.start.x = menuX + 50;       //button 1
    buttons.start.y = menuY + 200;
    buttons.start.width = 500;
    buttons.start.height = 100;
    XDrawRectangle(d, w, gc[0],
                   buttons.start.x, buttons.start.y, buttons.start.width, buttons.start.height);

    buttons.editor.x = menuX + 50;      //button 2
    buttons.editor.y = menuY + 350;
    buttons.editor.width = 500;
    buttons.editor.height = 100;
    XDrawRectangle(d, w, gc[0],
                   buttons.editor.x, buttons.editor.y, buttons.editor.width, buttons.editor.height);

    char textButton1[100] = "Start Game";
    XDrawString(d, w, gc[0], 470, 405, textButton1, (int)strlen(textButton1));

    char textButton2[100] = "Level Editor";
    XDrawString(d, w, gc[0], 465, 555, textButton2, (int)strlen(textButton2));

    char textTest[100] = "If text looks off center, it's because it probably is.";
    XDrawString(d, w, gc[0], 0, 12, textTest, (int)strlen(textTest));

    XFlush(d);
}

int menuLoop() {
    while (1)       //Main loop
    {
        XEvent e;
        XNextEvent(d, &e);
        printf("got event: %s\n", event_names[e.type]);     //Debug tool

        if (e.type == KeyPress) {
            char key = keyPressed(e);
            if (key == 27) {
                system("xset r on");
                XCloseDisplay(d);
                return 0;
            }
        }

        if (e.type == ButtonPress) {
            printf("Button XY = %i, %i\n", e.xbutton.x, e.xbutton.y);

            if (buttons.start.x < e.xbutton.x && e.xbutton.x < (buttons.start.x + buttons.start.width) &&
                buttons.start.y < e.xbutton.y && e.xbutton.y < (buttons.start.y + buttons.start.height))
                return 2;

            if (buttons.editor.x < e.xbutton.x && e.xbutton.x < (buttons.editor.x + buttons.editor.width) &&
                buttons.editor.y < e.xbutton.y && e.xbutton.y < (buttons.editor.y + buttons.editor.height))
                return 3;
        }
    }
}

int editorLoop() {

    XClearWindow(d, w);
    XStoreName(d, w, "Level editor?");

    char key = 0;
    int drawType = 0;       //  0 = draw, 1 = clear
    int boxX, boxY;
    struct Level level[35][167][80];
    int colour = 0;
    int firstFreeBox[35] = {0};

    //Read level from file and save to boxes[], if file doesn't exist clear array
    FILE *f = fopen("level.data", "rb");
    if (f != NULL) {
        fread(level, sizeof(struct Level), sizeof(level) / sizeof(struct Level), f);
        fclose(f);

        /*
        //get firstFreeBox from last element of array
        for (int i = 0; i < 35; i++) {
            for (int j = 5000; j >= 0; --j) {
                if (boxes[i][j].x != 0) {
                    firstFreeBox[i] = j+1;
                    break;
                }
            }
        }
         */
    } else {    //clear boxes array
        for (int i = 0; i < 35; i++) {
            for (int j = 0; j < 167; j++) {
                for (int k = 0; k < 80; k++) {
                    level[i][j][k].character = '\0';
                }
            }
        }
    }

    
    while (1) {      //Main loop
        drawEditor(level[part], drawType, colour);
        XEvent e;
        XNextEvent(d, &e);
        printf("got event: %s\n", event_names[e.type]);     //Debug tool

        if (e.type == KeyPress) {
            key = keyPressed(e);

            if (key == 27) {

                FILE *f = fopen("level.data", "wb");
                fwrite(level, sizeof(struct Level), sizeof(level) / sizeof(struct Level), f);
                fclose(f);

                system("xset r on");
                XCloseDisplay(d);
                return 0;
            }
        }

        if (e.type == ButtonPress) {
            if (900 < e.xbutton.x && e.xbutton.x < 950 &&      // cycle through colours
                0 < e.xbutton.y && e.xbutton.y < 50) {
                if (colour < 3)
                    colour += 1;
                else if (colour == 3)
                    colour = 0;

                XClearWindow(d, w);

            }
            else if (950 < e.xbutton.x && e.xbutton.x < 1000 &&  //Switch between clear and draw
                       0 < e.xbutton.y && e.xbutton.y < 50) {
                drawType = !drawType;
                XClearWindow(d, w);

            }
            else if (850 < e.xbutton.x && e.xbutton.x < 900 &&
                       0 < e.xbutton.y && e.xbutton.y < 50) {
                showMap(level);
                XClearWindow(d, w);

            }
            else if (800 < e.xbutton.x && e.xbutton.x < 850 &&
                        0 < e.xbutton.y && e.xbutton.y < 50) {
                showSpawner(level[part]);
            }
            else if (drawType == 0) {           //Try to find closest box to click ??

                boxX = ((e.xbutton.x - 2) / TEXTWIDTH) * TEXTWIDTH;     //TODO clean up
                boxY = ((e.xbutton.y + 5) / TEXTHEIGHT) * TEXTHEIGHT;
                level[part][boxX / TEXTWIDTH][boxY / TEXTHEIGHT].draw = True;
                level[part][boxX / TEXTWIDTH][boxY / TEXTHEIGHT].character = key;
                level[part][boxX / TEXTWIDTH][boxY / TEXTHEIGHT].gc = colour;
                level[part][boxX / TEXTWIDTH][boxY / TEXTHEIGHT].type = 0;

            } else if (drawType == 1) {

                boxX = ((e.xbutton.x - 2) / TEXTWIDTH) * TEXTWIDTH;
                boxY = ((e.xbutton.y + 5) / TEXTHEIGHT) * TEXTHEIGHT;
                level[part][boxX / TEXTWIDTH][boxY / TEXTHEIGHT].draw = False;
            }
        }
        if (e.type == MotionNotify) {
            if (drawType == 0) {           //Try to find closest box to click ??

                boxX = ((e.xbutton.x - 2) / TEXTWIDTH) * TEXTWIDTH;
                boxY = ((e.xbutton.y + 5) / TEXTHEIGHT) * TEXTHEIGHT;
                level[part][boxX / TEXTWIDTH][boxY / TEXTHEIGHT].draw = True;
                level[part][boxX / TEXTWIDTH][boxY / TEXTHEIGHT].character = key;
                level[part][boxX / TEXTWIDTH][boxY / TEXTHEIGHT].gc = colour;
                level[part][boxX / TEXTWIDTH][boxY / TEXTHEIGHT].type = 0;

            } else if (drawType == 1) {

                boxX = ((e.xbutton.x - 2) / TEXTWIDTH) * TEXTWIDTH;
                boxY = ((e.xbutton.y + 5) / TEXTHEIGHT) * TEXTHEIGHT;
                level[part][boxX / TEXTWIDTH][boxY / TEXTHEIGHT].draw = False;
            }
        }
    }
}

void drawEditor(struct Level level[167][80], int drawType, int colour) {

//  Draw/clear Button
    if (drawType == 1)
        XClearWindow(d, w);

    XDrawRectangle(d, w, gc[0], 950, 0, 49, 50);
    char textDraw[20] = "draw";
    char textClear[20] = "clear";

    if (drawType == 0)
        XDrawString(d, w, gc[0], 965, 30, textDraw, (int)strlen(textDraw));
    else if (drawType == 1)
        XDrawString(d, w, gc[0], 960, 30, textClear, (int)strlen(textClear));

    //  Colour button
    XDrawRectangle(d, w, gc[0], 900, 0, 50, 50);       //draw button
    char textColour[][20] = {"White", "grey", "red", "green"};
    XDrawString(d, w, gc[colour], 915, 30, textColour[colour], (int)strlen(textColour[colour]));

    //Map button
    XDrawRectangle(d, w, gc[0], 850, 0, 50, 50);
    char textType[20] = "Map";
    XDrawString(d, w, gc[0], 868, 30, textType, (int)strlen(textType));

    //spawner button
    XDrawRectangle(d, w, gc[0], 800, 0, 50, 50);
    char textSpawn[20] = "Items";
    XDrawString(d, w, gc[0], 810, 30, textSpawn, (int)strlen(textSpawn));

    for (int i = 0; i < 167; i++) {
        for (int j = 0; j < 80; j++) {
            if (level[i][j].draw == True) {
                XDrawString(d, w, gc[level[i][j].gc], i * TEXTWIDTH, j * TEXTHEIGHT, &level[i][j].character, 1);
            }
        }
    }
}

void showMap(struct Level level[35][167][80]) {

    //  Draw grid
    XClearArea(d, w, 150, 150, 700, 500, False);
    XDrawRectangle(d, w, gc[0], 150, 150, 700, 500);
    for (int i = 1; i < 5; i++) {
        XDrawLine(d, w, gc[0], 150, (150 + i * 100), 850, (150 + i * 100));
    }
    for (int i = 1; i < 7; i++) {
        XDrawLine(d, w, gc[0], (150 + i * 100), 150, (150 + i * 100), 650);
    }
    
    //  Make red box around any edited parts
    for (int i = 0; i < 35; i++) {
        for (int j = 0; j < 167; j++) {
            for (int k = 0; k < 80; k++) {
                if (level[i][j][k].draw == True) {
                    int x, y;
                    x = i % 7;
                    y = i / 7;
                    XDrawRectangle(d, w, gc[2], (150 + x*100), (150 + y*100), 100, 100);
                }
            }
        }
    }
    //  Draw green box around active part
    XDrawRectangle(d, w, gc[3], (150 + (part % 7)*100), (150 + (part / 7)*100), 100, 100);

    char text[20] = "start";
    XDrawString(d, w, gc[0], 185, 205, text, (int)strlen(text));

    for (int flag = 1; flag;) {

        XEvent e;
        XNextEvent(d, &e);

        //  check which box was clicked
        if (e.type == ButtonPress) {
            for (int i = 0; i < 5; i++) {
                for (int j = 0; j < 7; j++) {
                    if (e.xbutton.x > (150 + j * 100) && e.xbutton.x < (250 + j * 100) &&
                    e.xbutton.y > (150 + i * 100) && e.xbutton.y < (250 + i * 100)) {
                        part = i * 7 + j;
                        flag = !flag;
                    }
                }
            }
        }
        else if (e.type == KeyPress) {
            if (keyPressed(e) == 27) {
                return;
            }
        }
    }
}

int gameLoop() {

    XClearWindow(d, w);
    XStoreName(d, w, "Generic game?!");

    srand(time(NULL));
    struct Level level[35][167][80];
    //int firstFreeBox[35] = {0};
    int playerX = 500;
    int playerY = 400;
    struct timespec start, end, diff;
    int stopW = 0;
    int stopS = 0;
    int stopA = 0;
    int stopD = 0;
    int playerHasKey = 0;

    //Read level from file and save to level[]
    FILE *f = fopen("level.data", "rb");
    fread(level, sizeof(struct Level), sizeof(level) / sizeof(struct Level), f);
    fclose(f);

    drawLevel(level[part]);
    drawPlayer(playerX, playerY);

    clock_gettime(CLOCK_REALTIME, &start);
    char keys[3] = {'\0'};
    char lastKey[3] = {'d', '\0', '\0'};
    char key;
    XEvent e;
    XEvent eTrash;
    //Particle particles[256];
    int firstFreeParticle;
    char lastDirection = 'd';
    bulletNode bullets = NULL;

    while (1)       //Main loop
    {
        //  Only want player to move every 0.005s, so calculate time from last move, and sleep if less
        //  than 0.01s
        clock_gettime(CLOCK_REALTIME, &end);
        double timeDiff = (double) (end.tv_sec - start.tv_sec) + (double) (end.tv_nsec - start.tv_nsec) / BILLION;
        diff.tv_nsec = 0.01 * BILLION - (double) (end.tv_nsec - start.tv_nsec);

        if (timeDiff < 0.01)
            nanosleep(&diff, &diff);

        clock_gettime(CLOCK_REALTIME, &start);

        while (XCheckIfEvent(d, &eTrash, checkIfNotKeyEvent, NULL) == True);

        //  KeyReleaseEvent
        if (XCheckIfEvent(d, &e, checkIfKeysReleased, NULL) == True) {
            key = keyPressed(e);
            for (int i = 0; i < 3; i++) {   //remove released key from keys array
                if (key == keys[i]) {
                    keys[i] = '\0';
                }
            }
        }

        //  KeyPressEvent
        if (XCheckIfEvent(d, &e, checkIfKeyPress, NULL) == True) {
            key = keyPressed(e);

            //  Space key
            if (key == 32) {
                if (keys[0] == '\0' && keys[1] == '\0' && keys[2] == '\0') {
                    bullets = addBullet(bullets, playerX, playerY, lastKey);
                }
                else {
                    bullets = addBullet(bullets, playerX, playerY, keys);
                }
//                int counter = 0;
//                explosion(580, 400, &counter);
            }

            //  Escape key
            if (key == 27) {
                system("xset r on");
                XCloseDisplay(d);
                return 0;
            }

            if (key == 'w' || key == 'a' || key == 's' || key == 'd') {

                for (int i = 0; i < 3; i++) {
                    if (keys[i] == '\0') {
                        keys[i] = key;
                        lastKey[0] = key;
                        break;
                    }
                }
            }
        }

        if (keys[0] != '\0' || keys[1] != '\0' || keys[2] != '\0') {

            //Check if player is at borders, and switch scene
            if (playerX == 1000) {
                part += 1;
                playerX = 10;
                playerY = 400;
                drawLevel(level[part]);
                drawPlayer(playerX, playerY);

            } else if (playerX == 0) {
                part -= 1;
                playerX = 990;
                playerY = 400;
                drawLevel(level[part]);
                drawPlayer(playerX, playerY);

            } else if (playerY == 800) {
                part += 7;
                playerY = 10;
                playerX = 500;
                drawLevel(level[part]);
                drawPlayer(playerX, playerY);

            } else if (playerY == 0) {
                part -= 7;
                playerY = 770;
                playerX = 500;
                drawLevel(level[part]);
                drawPlayer(playerX, playerY);
            }

            //  Player Movement
            if (keys[0] == 'w' || keys[1] == 'w' || keys[2] == 'w') {
                stopW = 0;
                if (playerY % TEXTHEIGHT == 0) {
                    for (int i = 0; i < 4; i++) {   // Collision Detection
                        if (level[part][(playerX / TEXTWIDTH) + i][playerY / TEXTHEIGHT].draw == True) {
                            if (level[part][(playerX / TEXTWIDTH) + i][playerY / TEXTHEIGHT].type == 1 && drawDoor == 0) {
                                continue;
                            }
                            else if (level[part][(playerX / TEXTWIDTH) + i][playerY / TEXTHEIGHT].type == 2) {   //check if key
                                drawKey = 0;
                                drawDoor = 0;
                                XClearWindow(d,w);
                                drawLevel(level[part]);
                            }
                            else if (!(i == 3 && playerX % TEXTHEIGHT == 0))    // fix for not moving when touching walls
                                stopW = 1;
                        }
                    }
                }
                if (stopW == 0) {
                    playerY -= 2;
                    lastDirection = 'w';
                }
            }
            if (keys[0] == 's' || keys[1] == 's' || keys[2] == 's') {
                stopS = 0;
                if (playerY % TEXTHEIGHT == 0) {
                    for (int i = 0; i < 4; i++) {
                        if (level[part][(playerX / TEXTWIDTH) + i][(playerY / TEXTHEIGHT) + 5].draw == True) {
                            if (level[part][(playerX / TEXTWIDTH) + i][(playerY / TEXTHEIGHT) + 5].type == 1 && drawDoor == 0) {
                                continue;
                            }
                            else if (level[part][(playerX / TEXTWIDTH) + i][(playerY / TEXTHEIGHT) + 5].type == 2) {
                                drawKey = 0;
                                drawDoor = 0;
                                XClearWindow(d,w);
                                drawLevel(level[part]);
                            }
                            else if (!(i == 3 && playerX % TEXTHEIGHT == 0))
                                stopS = 1;
                        }
                    }
                }
                if (stopS == 0) {
                    playerY += 2;
                    lastDirection = 's';
                }
            }
            if (keys[0] == 'a' || keys[1] == 'a' || keys[2] == 'a') {
                stopA = 0;
                if (playerX % TEXTWIDTH == 0) {
                    for (int i = 0; i < 5; i++) {
                        if (level[part][(playerX / TEXTWIDTH) - 1][(playerY / TEXTHEIGHT) + i + 1].draw == True) {
                            if (level[part][(playerX / TEXTWIDTH) - 1][(playerY / TEXTHEIGHT) + i + 1].type == 1 && drawDoor == 0) {
                                continue;
                            }
                            else if (level[part][(playerX / TEXTWIDTH) - 1][(playerY / TEXTHEIGHT) + i + 1].type == 2) {
                                drawKey = 0;
                                drawDoor = 0;
                                XClearWindow(d,w);
                                drawLevel(level[part]);
                            }
                            else if (!(i == 4 && playerY % TEXTHEIGHT == 0))
                                stopA = 1;
                        }
                    }
                }
                if (stopA == 0) {
                    playerX -= 2;
                    lastDirection = 'a';
                }
            }
            if (keys[0] == 'd' || keys[1] == 'd' || keys[2] == 'd') {
                stopD = 0;
                if (playerX % TEXTWIDTH == 0) {
                    for (int i = 0; i < 5; i++) {
                        if (level[part][(playerX / TEXTWIDTH) + 3][(playerY / TEXTHEIGHT) + i + 1].draw == True) {
                            if (level[part][(playerX / TEXTWIDTH) + 3][(playerY / TEXTHEIGHT) + i + 1].type == 1 && drawDoor == 0) {
                                continue;
                            }
                            else if (level[part][(playerX / TEXTWIDTH) + 3][(playerY / TEXTHEIGHT) + i + 1].type == 2) {
                                drawKey = 0;
                                drawDoor = 0;
                                XClearWindow(d,w);
                                drawLevel(level[part]);
                            }
                            else if (!(i == 4 && playerY % TEXTHEIGHT == 0))
                                stopD = 1;
                        }
                    }
                }
                if (stopD == 0) {
                    playerX += 2;
                    lastDirection = 'd';
                }
            }
        }
        drawPlayer(playerX, playerY);
        bullets = bulletTick(bullets, level[part]);
        XFlush(d);
    }
}

bulletNode addBullet(bulletNode bullets, int playerX, int playerY, char keys[3]) {
    bulletNode temp, counter;
    temp = createBulletNode();
    temp->x = playerX + TEXTWIDTH * 2;
    temp->y = playerY + TEXTHEIGHT * 2;
    temp->keys[0] = keys[0];
    temp->keys[1] = keys[1];
    temp->keys[2] = keys[2];

    if(bullets == NULL){
        bullets = temp;    //if list is empty, set first value to temp, else append to end of list.
    }
    else{
        counter = bullets;
        while(counter->next != NULL){
            counter = counter->next;
        }
        counter->next = temp;
    }
    return bullets;
}

bulletNode bulletTick(bulletNode bullets, struct Level level[167][80]) {
    bulletNode temp, prev, freeNode;
    char bulletText = '*';
    freeNode = NULL;
    prev = NULL;
    temp = bullets;
    while (temp != NULL) {
        while (1) {
            XClearArea(d, w, temp->x, temp->y - TEXTHEIGHT, TEXTWIDTH, TEXTHEIGHT, 0);
            if (temp->keys[0] == 'w' || temp->keys[1] == 'w' || temp->keys[2] == 'w') {
                int i = 0;
                if (i < 2) {        //Collision detection
                    i++;
                    if (level[temp->x / TEXTWIDTH + i][temp->y / TEXTHEIGHT - 1].draw == True) {
                        if (prev == NULL) {  //remove node from list
                            freeNode = temp;
                            bullets = temp->next;
                            temp = temp->next;
                            free(freeNode);
                            break;
                        } else {
                            prev->next = temp->next;
                            freeNode = temp;
                            temp = temp->next;
                            free(freeNode);
                            break;
                        }
                    }
                }
                if (temp->y > 5) {      //Check if bullet is within screen borders
                    temp->y -= 5;
                }
                else {
                    if (prev == NULL) { //remove node from list
                        freeNode = temp;
                        bullets = temp->next;
                        temp = temp->next;
                        free(freeNode);
                        break;
                    } else {
                        prev->next = temp->next;
                        freeNode = temp;
                        temp = temp->next;
                        free(freeNode);
                        break;
                    }
                }
            }

            if (temp->keys[0] == 's' || temp->keys[1] == 's' || temp->keys[2] == 's') {
                int i = 0;
                if (i < 2) {    //Collision detection
                    i++;
                    if (level[temp->x / TEXTWIDTH + i][temp->y / TEXTHEIGHT + 1].draw == True) {
                        if (prev == NULL) {  //remove node from list
                            freeNode = temp;
                            bullets = temp->next;
                            temp = temp->next;
                            free(freeNode);
                            break;
                        } else {
                            prev->next = temp->next;
                            freeNode = temp;
                            temp = temp->next;
                            free(freeNode);
                            break;
                        }
                    }
                }
                if (temp->y < 800) {
                    temp->y += 5;
                }
                else {
                    if (prev == NULL) {
                        freeNode = temp;
                        bullets = temp->next;
                        temp = temp->next;
                        free(freeNode);
                        break;
                    } else {
                        prev->next = temp->next;
                        freeNode = temp;
                        temp = temp->next;
                        free(freeNode);
                        break;
                    }
                }
            }

            if (temp->keys[0] == 'a' || temp->keys[1] == 'a' || temp->keys[2] == 'a') {
                int i = 0;
                if (i < 2) {
                    i++;
                    if (level[temp->x / TEXTWIDTH - 1][temp->y / TEXTHEIGHT + i - 1].draw == True) {
                        if (prev == NULL) {
                            freeNode = temp;
                            bullets = temp->next;
                            temp = temp->next;
                            free(freeNode);
                            break;
                        } else {
                            prev->next = temp->next;
                            freeNode = temp;
                            temp = temp->next;
                            free(freeNode);
                            break;
                        }
                    }
                }
                if (temp->x > 5) {
                    temp->x -= 5;
                }
                else {
                    if (prev == NULL) {
                        freeNode = temp;
                        bullets = temp->next;
                        temp = temp->next;
                        free(freeNode);
                        break;
                    } else {
                        prev->next = temp->next;
                        freeNode = temp;
                        temp = temp->next;
                        free(freeNode);
                        break;
                    }
                }
            }
            if (temp->keys[0] == 'd' || temp->keys[1] == 'd' || temp->keys[2] == 'd') {
                int i = 0;
                if (i < 2) {
                    i++;
                    if (level[temp->x / TEXTWIDTH + 1][temp->y / TEXTHEIGHT + i - 1].draw == True) {
                        if (prev == NULL) {
                            freeNode = temp;
                            bullets = temp->next;
                            temp = temp->next;
                            free(freeNode);
                            break;
                        } else {
                            prev->next = temp->next;
                            freeNode = temp;
                            temp = temp->next;
                            free(freeNode);
                            break;
                        }
                    }
                }
                if (temp->x < 1000) {
                    temp->x += 5;
                }
                else {
                    if (prev == NULL) {
                        freeNode = temp;
                        bullets = temp->next;
                        temp = temp->next;
                        free(freeNode);
                        break;
                    } else {
                        prev->next = temp->next;
                        freeNode = temp;
                        temp = temp->next;
                        free(freeNode);
                        break;
                    }
                }
            }

            XDrawString(d, w, gc[1], temp->x, temp->y, &bulletText, 1);
            prev = temp;
            temp = temp->next;
            break;
        }
    }
    return bullets;
}

/*
void explosion(int x, int y, int *counter) {
    int particleCount = 5;
    char letter[2] = "*";
    double gravity = 0.2;
//    int minVel = 1;
//    int maxVel = 2;
    double xVel[5];
    double yVel[5];
    int particleX[5];
    int particleY[5];


    //  Give random initial velocity
    if (*counter == 0) {
        for (int i = 0; i < particleCount; i++) {
            if (rand() > RAND_MAX/2)
                xVel[i] = ((double)rand()/(double)(RAND_MAX))*3;
            else
                xVel[i] = -((double)rand()/(double)(RAND_MAX))*3;

            yVel[i] = ((double)rand()/(double)(RAND_MAX))*6;
        }
    }

    //temp
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 0.005 * BILLION;

    while (*counter < 50) {
        //suvat s=vt
        for (int i = 0; i < particleCount; i++) {
            particleX[i] = x + xVel[i] * *counter;
            particleY[i] = y - (yVel[i] * *counter) + (0.5 * gravity * *counter * *counter);
            XDrawString(d, w, gc[3], particleX[i], particleY[i], letter, (int)strlen(letter));
        }
        *counter += 1;
        XFlush(d);
        nanosleep(&time, &time);
    }
}
 */

void showSpawner(struct Level level[167][80]) {
    // Show item menu
    XClearArea(d, w, 800, 50, 50, 100, False);
    XDrawRectangle(d, w, gc[0], 800, 50, 50, 50);
    char tempText[] = "key";
    XDrawString(d, w, gc[2], 815, 80, tempText, (int)strlen(tempText));
    XDrawRectangle(d, w, gc[0], 800, 100, 50, 50);
    char doorText[] = "door";
    XDrawString(d, w, gc[2], 815, 130, doorText, (int)strlen(doorText));
    int thing = 0;

    while(1) {
        XEvent e;
        XNextEvent(d, &e);

        if (e.type == ButtonPress) {
            switch (thing) {
                case 0: {
                    if (800 < e.xbutton.x && e.xbutton.x < 850 &&
                        50 < e.xbutton.y && e.xbutton.y < 100) {        //check if Key Button pressed
                        XDrawRectangle(d, w, gc[3], 800, 50, 50, 50);
                        thing = 1;
                        break;
                    } else if (800 < e.xbutton.x && e.xbutton.x < 850 &&
                               100 < e.xbutton.y && e.xbutton.y < 150) {
                        XDrawRectangle(d, w, gc[3], 800, 100, 50, 50);
                        thing = 2;
                        break;
                    } else {
                        return;
                    }
                }
                case 1: {       // Add key to level array on button press position
                    char keyText[5] = "O---W";
                    for (int i = 0; i < 5; i++) {
                        int boxX = (((e.xbutton.x - 2) / TEXTWIDTH) * TEXTWIDTH) + i * TEXTWIDTH;     //TODO clean up
                        int boxY = (((e.xbutton.y + 5) / TEXTHEIGHT) * TEXTHEIGHT);
                        level[boxX / TEXTWIDTH][boxY / TEXTHEIGHT].draw = True;
                        level[boxX / TEXTWIDTH][boxY / TEXTHEIGHT].character = keyText[i];
                        level[boxX / TEXTWIDTH][boxY / TEXTHEIGHT].gc = 2;
                        level[boxX / TEXTWIDTH][boxY / TEXTHEIGHT].type = 2;
                    }
                    return;
                }
                case 2: {   // Add wall to level
                    for (int i = 0; i < 5; i++) {
                        int boxX = (((e.xbutton.x - 2) / TEXTWIDTH) * TEXTWIDTH);     //TODO clean up
                        int boxY = (((e.xbutton.y + 5) / TEXTHEIGHT) * TEXTHEIGHT) + i * TEXTHEIGHT;
                        level[boxX / TEXTWIDTH][boxY / TEXTHEIGHT].draw = True;
                        level[boxX / TEXTWIDTH][boxY / TEXTHEIGHT].character = '#';
                        level[boxX / TEXTWIDTH][boxY / TEXTHEIGHT].gc = 2;
                        level[boxX / TEXTWIDTH][boxY / TEXTHEIGHT].type = 1;
                    }
                    return;
                }
            }
        } else
            return;
    }
}

Bool checkIfKeysReleased(Display *display, XEvent *e, XPointer arg) {
    printf("got event: %s\n", event_names[e->type]);

    if (e->type == 3)
        return True;
    else
        return False;
}

Bool checkIfKeyPress(Display *display, XEvent *e, XPointer keys) {

    if (e->type == 2)
        return True;
    else
        return False;
}

Bool checkIfNotKeyEvent(Display *display, XEvent *e, XPointer arg) {
    if (e->type != 2 && e->type != 3)
        return True;
    else
        return False;
}

void drawLevel(struct Level level[167][80]) {
    XClearWindow(d, w);

    if (drawKey == 0 && drawDoor == 0) {
        for (int i = 0; i < 167; i++) {
            for (int j = 0; j < 80; j++) {
                if (level[i][j].draw == True && level[i][j].type == 0) {
                    XDrawString(d, w, gc[level[i][j].gc], i * TEXTWIDTH, j * TEXTHEIGHT, &level[i][j].character, 1);
                }
            }
        }
    }
    else if (drawKey == 1 && drawDoor == 1) {
        for (int i = 0; i < 167; i++) {
            for (int j = 0; j < 80; j++) {
                if (level[i][j].draw == True) {
                    XDrawString(d, w, gc[level[i][j].gc], i * TEXTWIDTH, j * TEXTHEIGHT, &level[i][j].character, 1);
                }
            }
        }
    }
    else if (drawKey == 0 && drawDoor == 1) {
        for (int i = 0; i < 167; i++) {
            for (int j = 0; j < 80; j++) {
                if (level[i][j].draw == True && (level[i][j].type == 0 || level[i][j].type == 1)) {
                    XDrawString(d, w, gc[level[i][j].gc], i * TEXTWIDTH, j * TEXTHEIGHT, &level[i][j].character, 1);
                }
            }
        }
    }
}

void drawPlayer(int x, int y) {

    if (oldPlayerX && oldPlayerY)
        XClearArea(d, w, oldPlayerX, oldPlayerY, TEXTWIDTH * 3, TEXTHEIGHT * 4, False);

    for (int i = 0; i < 7; i++) {
        XDrawString(d, w, gc[0], (player[i].x + x), (player[i].y + y), &player[i].character, 1);
    }

    oldPlayerX = x;
    oldPlayerY = y;
}