#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>
#include <time.h>

#define TEXTHEIGHT 10
#define TEXTWIDTH 6
#define BILLION 1E9


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

struct Button {
    int x, y, width, height;
};

struct MenuButtons {
    struct Button start, editor;
};

struct Box {
    int x, y, exit, gc;
    char character;
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

char keyPressed(XEvent e);
void drawMenu();
void drawLevel(struct Box *boxes, int firstFreeBox);
void drawEditor(struct Box *boxes, int firstFreeBox, int drawType, int colour);
void showMap(int firstFreeBox[]);
void drawPlayer(int x, int y);
int menuLoop();
int gameLoop();
int editorLoop();
Bool CheckIfKeyRelease(Display *d, XEvent *e, XPointer arg);


int main(void) {

    system("xset r off");      //Disable Key Auto repeat - global across client (spooky)

    d = XOpenDisplay(NULL);

    int screen = DefaultScreen(d);
    int black = BlackPixel(d, screen);
    int white = WhitePixel(d, screen);

    w = XCreateSimpleWindow(d, RootWindow(d, screen),
                            10, 10, 1000, 800, 0, white, black);

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

char keyPressed(XEvent e) {      //Returns key pressed

    char letter[1];
    KeySym key = 0;
    XLookupString(&e.xkey, letter, sizeof(char), &key, 0);
    printf("key = %d\n", *letter);
    return letter[0];
}

void drawMenu() {      //Displays menu, including buttons and text
    XClearWindow(d, w);
    XStoreName(d, w, "what");

    int menuX = 200;
    int menuY = 150;
    XDrawRectangle(d, w, gc[0], menuX, menuY, 600, 500);     //Big box

    for (int i = 0; i < 5; i++) {
        XDrawString(d, w, gc[0], 300, (240 + 10 * i), title[i], strlen(title[i]));
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
    XDrawString(d, w, gc[0], 470, 405, textButton1, strlen(textButton1));

    char textButton2[100] = "Level Editor";
    XDrawString(d, w, gc[0], 465, 555, textButton2, strlen(textButton2));

    char textTest[100] = "If text looks off center, it's because it probably is.";
    XDrawString(d, w, gc[0], 0, 12, textTest, strlen(textTest));

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
    struct Box boxes[35][5000];
    int colour = 0;
    int firstFreeBox[35] = {0};

    //Read level from file and save to boxes[], if file doesn't exist clear array
    FILE *f = fopen("level.data", "rb");
    if (f != NULL) {
        fread(boxes, sizeof(struct Box), sizeof(boxes) / sizeof(struct Box), f);
        fclose(f);

        //get firstFreeBox from last element of array
        for (int i = 0; i < 35; i++) {
            for (int j = 5000; j >= 0; --j) {
                if (boxes[i][j].x != 0) {
                    firstFreeBox[i] = boxes[i][j].x;
                    break;
                }
            }
        }
    } else {
        for (int i = 0; i < 35; i++) {
            for (int j = 0; j < 5000; j++) {        //clear boxes array
                boxes[i][j].x = 0;
                boxes[i][j].y = 0;
                boxes[i][j].character = '\0';
            }
        }
    }






    while (1) {      //Main loop
        drawEditor(boxes[part], firstFreeBox[part], drawType, colour);
        XEvent e;
        XNextEvent(d, &e);
        printf("got event: %s\n", event_names[e.type]);     //Debug tool

        if (e.type == KeyPress) {
            key = keyPressed(e);

            if (key == 27) {

                for (int i = 0; i < 35; i++) {
                    boxes[i][firstFreeBox[i]].x = firstFreeBox[i];  //Append firstFreeBox to list, so it can be read later
                }
                FILE *f = fopen("level.data", "wb");
                fwrite(boxes, sizeof(struct Box), sizeof(boxes) / sizeof(struct Box), f);
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

            } else if (950 < e.xbutton.x && e.xbutton.x < 1000 &&  //Switch between clear and draw
                       0 < e.xbutton.y && e.xbutton.y < 50) {
                drawType = !drawType;
                XClearWindow(d, w);

            } else if (850 < e.xbutton.x && e.xbutton.x < 900 &&
                       0 < e.xbutton.y && e.xbutton.y < 50) {
                showMap(firstFreeBox);
                XClearWindow(d, w);

            } else if (drawType == 0) {           //Try to find closest box to click ??

                boxX = ((e.xbutton.x - 2) / TEXTWIDTH) * TEXTWIDTH;
                boxY = ((e.xbutton.y + 5) / TEXTHEIGHT) * TEXTHEIGHT;

                for (int i = 0; i <= firstFreeBox[part]; i++) {
                    if (boxes[part][i].x == boxX && boxes[part][i].y == boxY) {
                        break;

                    } else if (i == firstFreeBox[part]) {
                        boxes[part][firstFreeBox[part]].x = boxX;
                        boxes[part][firstFreeBox[part]].y = boxY;
                        boxes[part][firstFreeBox[part]].character = key;
                        boxes[part][firstFreeBox[part]].gc = colour;
                        firstFreeBox[part] += 1;
                        break;
                    }
                }
            } else if (drawType == 1) {

                boxX = ((e.xbutton.x - 2) / TEXTWIDTH) * TEXTWIDTH;
                boxY = ((e.xbutton.y + 5) / TEXTHEIGHT) * TEXTHEIGHT;

                for (int i = 0; i < firstFreeBox[part]; i++) {
                    if (boxes[part][i].x == boxX && boxes[part][i].y == boxY) {
                        firstFreeBox[part] -= 1;
                        boxes[part][i] = boxes[part][firstFreeBox[part]];
                        break;
                    }
                }
            }
        }
        if (e.type == MotionNotify) {
            if (drawType == 0) {           //Try to find closest box to click ??

                boxX = ((e.xbutton.x - 2) / TEXTWIDTH) * TEXTWIDTH;
                boxY = ((e.xbutton.y + 5) / TEXTHEIGHT) * TEXTHEIGHT;

                for (int i = 0; i <= firstFreeBox[part]; i++) {
                    if (boxes[part][i].x == boxX && boxes[part][i].y == boxY) {
                        break;
                    } else if (i == firstFreeBox[part]) {
                        boxes[part][firstFreeBox[part]].x = boxX;
                        boxes[part][firstFreeBox[part]].y = boxY;
                        boxes[part][firstFreeBox[part]].character = key;
                        boxes[part][firstFreeBox[part]].gc = colour;
                        firstFreeBox[part] += 1;
                        break;
                    }
                }
            } else if (drawType == 1) {

                boxX = ((e.xbutton.x - 2) / TEXTWIDTH) * TEXTWIDTH;
                boxY = ((e.xbutton.y + 5) / TEXTHEIGHT) * TEXTHEIGHT;

                for (int i = 0; i < firstFreeBox[part]; i++) {
                    if (boxes[part][i].x == boxX && boxes[part][i].y == boxY) {
                        firstFreeBox[part] -= 1;
                        boxes[part][i] = boxes[part][firstFreeBox[part]];
                        break;
                    }
                }
            }
        }
    }
}

void drawEditor(struct Box *boxes, int firstFreeBox, int drawType, int colour) {

//  Draw/clear Button
    if (drawType == 1)
        XClearWindow(d, w);

    XDrawRectangle(d, w, gc[0], 950, 0, 49, 50);
    char textDraw[20] = "draw";
    char textClear[20] = "clear";

    if (drawType == 0)
        XDrawString(d, w, gc[0], 965, 30, textDraw, strlen(textDraw));
    else if (drawType == 1)
        XDrawString(d, w, gc[0], 960, 30, textClear, strlen(textClear));

    //  Colour button
    XDrawRectangle(d, w, gc[0], 900, 0, 50, 50);       //draw button
    char textColour[][20] = {"White", "grey", "red", "green"};
    XDrawString(d, w, gc[colour], 915, 30, textColour[colour], strlen(textColour[colour]));

    //Map button
    XDrawRectangle(d, w, gc[0], 850, 0, 50, 50);
    char textType[20] = "Map";
    XDrawString(d, w, gc[0], 868, 30, textType, strlen(textType));

    for (int i = 0; i < firstFreeBox; i++) {
        XDrawString(d, w, gc[boxes[i].gc], boxes[i].x, boxes[i].y, &boxes[i].character, 1);
    }

    /*
    for (int i = 0; i < firstFreeBox; i++) {
        XDrawRectangle(d, w, gc, (boxes[i].x), (boxes[i].y), 6, 12);
    }
     */
}

void showMap(int firstFreeBox[]) {

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
        if (firstFreeBox[i]) {
            int x, y;
            x = i % 7;
            y = i / 7;
            XDrawRectangle(d, w, gc[2], (150 + x*100), (150 + y*100), 100, 100);
        }
    }
    //  Draw green box around active part
    XDrawRectangle(d, w, gc[3], (150 + (part % 7)*100), (150 + (part / 7)*100), 100, 100);

    char text[20] = "start";
    XDrawString(d, w, gc[0], 185, 205, text, strlen(text));

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

    struct Box boxes[35][5000];
    int firstFreeBox[35] = {0};
    int playerX = 500;
    int playerY = 400;
    struct timespec start, end, diff;
    int stopW = 0;
    int stopS = 0;
    int stopA = 0;
    int stopD = 0;

    //Read level from file and save to boxes[]
    FILE *f = fopen("level.data", "rb");
    fread(boxes, sizeof(struct Box), sizeof(boxes) / sizeof(struct Box), f);
    fclose(f);

    //get firstFreeBox from last element of array
    for (int i = 0; i < 35; i++) {
        for (int j = 5000; j >= 0; --j) {
            if (boxes[i][j].x != 0) {
                firstFreeBox[i] = boxes[i][j].x;
                break;
            }
        }
    }


    while (1)       //Main loop
    {
        drawLevel(boxes[part], firstFreeBox[part]);
        drawPlayer(playerX, playerY);

        XEvent e;
        XNextEvent(d, &e);
        printf("got event: %s\n", event_names[e.type]);     //Debug tool

        if (e.type == KeyPress) {
            clock_gettime(CLOCK_REALTIME, &start);
            char key = keyPressed(e);

            if (key == 27) {
                system("xset r on");
                XCloseDisplay(d);
                return 0;
            }
            if ((key == 'w') || (key == 'a') || (key == 's') || (key == 'd')) {

                //Loop until keyRelease event of that key
                while (XCheckIfEvent(d, &e, CheckIfKeyRelease, &key) == False) {

                    //  Only want player to move every 0.005s, so calculate time from last move, and sleep if less
                    //  than 0.005s
                    clock_gettime(CLOCK_REALTIME, &end);
                    double timeDiff = (end.tv_sec - start.tv_sec) + ((end.tv_nsec - start.tv_nsec) / BILLION);
                    diff.tv_nsec = 0.005*BILLION - (end.tv_nsec - start.tv_nsec);

                    /*
                                * Bottom edge of box
                                *
                                *(box.x, box.y) to (box.x + WIDTH, box.y)
                                *{
                                * playerX and PlayerY are from top left of player
                                * Box.x and box.y are from bottom left of box
                                *
                                * playerX, PlayerY
                                */

                    if (timeDiff < 0.005)
                        nanosleep(&diff, &diff);

                    //Check if player is at borders, and switch scene
                    if (playerX == 1000) {
                        part += 1;
                        playerX = 10;
                        playerY = 400;
                        drawLevel(boxes[part], firstFreeBox[part]);
                        drawPlayer(playerX, playerY);

                    } else if (playerX == 0) {
                        part -= 1;
                        playerX = 990;
                        playerY = 400;
                        drawLevel(boxes[part], firstFreeBox[part]);
                        drawPlayer(playerX, playerY);

                    } else if (playerY == 800) {
                        part += 7;
                        playerY = 10;
                        playerX = 500;
                        drawLevel(boxes[part], firstFreeBox[part]);
                        drawPlayer(playerX, playerY);

                    } else if (playerY == 0) {
                        part -= 7;
                        playerY = 770;
                        playerX = 500;
                        drawLevel(boxes[part], firstFreeBox[part]);
                        drawPlayer(playerX, playerY);
                    }

                    if (key == 'w') {

                        stopW = 0;
                        for (int i = 0; i < firstFreeBox[part]; i++) {
                            if (boxes[part][i].y == playerY &&
                                boxes[part][i].x < (playerX + TEXTWIDTH * 3) &&
                                (boxes[part][i].x + TEXTWIDTH) > playerX) {
                                stopW = 1;
                            }
                        }
                        if (stopW == 0)
                            playerY -= 1;

                    } else if (key == 's') {

                        stopS = 0;
                        for (int i = 0; i < firstFreeBox[part]; i++) {
                            if ((boxes[part][i].y - TEXTHEIGHT) == (playerY + TEXTHEIGHT * 4) &&
                                boxes[part][i].x < (playerX + TEXTWIDTH * 3) &&
                                (boxes[part][i].x + TEXTWIDTH) > playerX) {
                                stopS = 1;
                            }
                        }
                        if (stopS == 0)
                            playerY += 1;

                    } else if (key == 'a') {
                        stopA = 0;
                        for (int i = 0; i < firstFreeBox[part]; i++) {
                            if ((boxes[part][i].x + TEXTWIDTH) == playerX &&
                                boxes[part][i].y > playerY &&
                                (boxes[part][i].y - TEXTHEIGHT) < (playerY + TEXTHEIGHT * 4)) {
                                stopA = 1;
                            }
                        }
                        if (stopA == 0)
                            playerX -= 1;

                    } else if (key == 'd') {
                        stopD = 0;
                        for (int i = 0; i < firstFreeBox[part]; i++) {
                            if (boxes[part][i].x == (playerX + TEXTWIDTH * 3) &&
                                boxes[part][i].y > playerY &&
                                (boxes[part][i].y - TEXTHEIGHT) < (playerY + TEXTHEIGHT * 4)) {
                                stopD = 1;
                            }
                        }
                        if (stopD == 0)
                            playerX += 1;
                    }


                    clock_gettime(CLOCK_REALTIME, &start);
                    drawPlayer(playerX, playerY);
                }
            }
        }
    }
}

Bool CheckIfKeyRelease(Display *display, XEvent *event, XPointer arg) {

    if (event->type == 3 && (keyPressed(*event) == arg[0]))
        return True;
    else
        return False;
}

void drawLevel(struct Box *boxes, int firstFreeBox) {

    XClearWindow(d, w);

    for (int i = 0; i < firstFreeBox; i++) {
        XDrawString(d, w, gc[boxes[i].gc], boxes[i].x, boxes[i].y, &boxes[i].character, 1);
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




//TODOS
/*
 * DONE - flickering on drawing, don't clear all ?????
 *
 * DONE - Change other button to colour switch
 *
 * CHANGED - Functions need GC pointer to list, not single gc?
 *
 * DONE - Add drawing of correct colours
 *
 * DONE - add player sprite
 *
 * DONE - add movement - xcleararea can create event?
 *
 * DONE  - collision detection
 *
 * Add multiple levels to the editor
 *
 * Add type button in editor, add exit type
 *
 * detect type on collision, teleport
 *
 * improve editor, allow for multiple rooms....
 *
 * brush size
 *
 * etc
 */