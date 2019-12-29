#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>
#include <time.h>

#define TEXTHEIGHT 10
#define TEXTWIDTH 6
#define BILLION 1E9


static const char *event_names[] = {
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

char title[][100] = {       // ASCII Gen = http://patorjk.com/software/taag/
        "  __ _  ___ _ __   ___ _ __(_) ___    __ _  __ _ _ __ ___   ___",
        " / _` |/ _ \\ '_ \\ / _ \\ '__| |/ __|  / _` |/ _` | '_ ` _ \\ / _ \\",
        "| (_| |  __/ | | |  __/ |  | | (__  | (_| | (_| | | | | | |  __/",
        " \\__, |\\___|_| |_|\\___|_|  |_|\\___|  \\__, |\\__,_|_| |_| |_|\\___|",
        " |___/                               |___/"
};

struct Button {
    int x, y, width, height;
};

struct Buttons {
    struct Button start, editor;
};

struct Box {
    int x, y, exit, gc;
    char character;
};

struct Box player[7] = {{6,  10, 0, 0, 79},
                        {6,  20, 0, 0, 124},
                        {6,  30, 0, 0, 124},
                        {12, 40, 0, 0, 92},
                        {12, 20, 0, 0, 92},
                        {0,  40, 0, 0, 47},
                        {0,  20, 0, 0, 47}};

struct Buttons buttons;
Display *d;
Window w;
GC gc[4];
XColor xc_grey, xc_red, xc_green;
Colormap c;
int oldPlayerX = 0;
int oldPlayerY = 0;

char keyPressed(XEvent e);

void drawMenu();

void drawLevel(struct Box *boxes, int firstFreeBox);

void drawEditor(struct Box *boxes, int firstFreeBox, int drawType, int colour);

void drawPlayer(int x, int y);

int menuLoop();

int gameLoop();

int editorLoop();

Bool CheckIfKeyRelease(Display *d, XEvent *e, XPointer arg);


int main(void) {

    system("xset r off");      //Disable Key Auto repeat - global across client (spooky)

    d = XOpenDisplay(NULL);

    if (d == NULL) {
        printf("Cannot open d\n");
        exit(0);
    }

    int screen = DefaultScreen(d);
    int black = BlackPixel(d, screen);
    int white = WhitePixel(d, screen);

    w = XCreateSimpleWindow(d, RootWindow(d, screen),
                            10, 10, 1000, 800, 0, white, black);

    XSelectInput(d, w,
                 StructureNotifyMask | KeyPressMask | ExposureMask | KeyReleaseMask | ButtonPressMask |
                 ButtonMotionMask); // NOLINT(hicpp-signed-bitwise)
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

    while (1)       //Wait for w to be mapped before starting main loop
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

    char letter[32];
    for (int i = 0; i < strlen(letter); i++)
        letter[i] = "";

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

    //XDrawLine(d, w, gc[0], 200, 100, 200, 0);
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
    struct Box boxes[5000];

    int colour = 0;
    int firstFreeBox = 0;
    for (int i = 0; i < 5000; i++) {        //clear boxes array
        boxes[i].x = 0;
        boxes[i].y = 0;
        boxes[i].character = '\0';
    }


    while (1)       //Main loop
    {
        drawEditor(boxes, firstFreeBox, drawType, colour);
        XEvent e;
        XNextEvent(d, &e);
        printf("got event: %s\n", event_names[e.type]);     //Debug tool

        if (e.type == KeyPress) {
            key = keyPressed(e);

            if (key == 27) {

                boxes[firstFreeBox].x = firstFreeBox;  //Append firstFreeBox to list, so it can be read later
                FILE *f = fopen("level.data", "wb");
                fwrite(boxes, sizeof(struct Box), 5000, f);
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
            } else if (drawType == 0) {           //Try to find closest box to click ??

                boxX = ((e.xbutton.x - 2) / TEXTWIDTH) * TEXTWIDTH;
                boxY = ((e.xbutton.y + 5) / TEXTHEIGHT) * TEXTHEIGHT;

                for (int i = 0; i <= firstFreeBox; i++) {
                    if (boxes[i].x == boxX && boxes[i].y == boxY) {
                        break;
                    } else if (i == firstFreeBox) {
                        boxes[firstFreeBox].x = boxX;
                        boxes[firstFreeBox].y = boxY;
                        boxes[firstFreeBox].character = key;
                        boxes[firstFreeBox].gc = colour;
                        firstFreeBox += 1;
                        break;
                    }
                }
            } else if (drawType == 1) {

                boxX = ((e.xbutton.x - 2) / TEXTWIDTH) * TEXTWIDTH;
                boxY = ((e.xbutton.y + 5) / TEXTHEIGHT) * TEXTHEIGHT;

                for (int i = 0; i < firstFreeBox; i++) {
                    if (boxes[i].x == boxX && boxes[i].y == boxY) {
                        firstFreeBox -= 1;
                        boxes[i] = boxes[firstFreeBox];
                        break;
                    }
                }
            }
        }
        if (e.type == MotionNotify) {
            if (drawType == 0) {           //Try to find closest box to click ??

                boxX = ((e.xbutton.x - 2) / TEXTWIDTH) * TEXTWIDTH;
                boxY = ((e.xbutton.y + 5) / TEXTHEIGHT) * TEXTHEIGHT;

                for (int i = 0; i <= firstFreeBox; i++) {
                    if (boxes[i].x == boxX && boxes[i].y == boxY) {
                        break;
                    } else if (i == firstFreeBox) {
                        boxes[firstFreeBox].x = boxX;
                        boxes[firstFreeBox].y = boxY;
                        boxes[firstFreeBox].character = key;
                        boxes[firstFreeBox].gc = colour;
                        firstFreeBox += 1;
                        break;
                    }
                }
            } else if (drawType == 1) {

                boxX = ((e.xbutton.x - 2) / TEXTWIDTH) * TEXTWIDTH;
                boxY = ((e.xbutton.y + 5) / TEXTHEIGHT) * TEXTHEIGHT;

                for (int i = 0; i < firstFreeBox; i++) {
                    if (boxes[i].x == boxX && boxes[i].y == boxY) {
                        firstFreeBox -= 1;
                        boxes[i] = boxes[firstFreeBox];
                        break;
                    }
                }
            }
        }
    }
}


void drawEditor(struct Box *boxes, int firstFreeBox, int drawType, int colour) {


    if (drawType == 1)
        XClearWindow(d, w);

    XDrawRectangle(d, w, gc[0], 950, 0, 49, 50);
    char textDraw[20] = "draw";
    char textClear[20] = "clear";

    if (drawType == 0)
        XDrawString(d, w, gc[0], 965, 30, textDraw, strlen(textDraw));
    else if (drawType == 1)
        XDrawString(d, w, gc[0], 960, 30, textClear, strlen(textClear));


    XDrawRectangle(d, w, gc[0], 900, 0, 50, 50);       //draw button
    char textColour[][20] = {"White", "grey", "red", "green"};
    XDrawString(d, w, gc[colour], 915, 30, textColour[colour], strlen(textColour[colour]));

    for (int i = 0; i < firstFreeBox; i++) {
        XDrawString(d, w, gc[boxes[i].gc], boxes[i].x, boxes[i].y, &boxes[i].character, 1);
    }

    /*
    for (int i = 0; i < firstFreeBox; i++) {
        XDrawRectangle(d, w, gc, (boxes[i].x), (boxes[i].y), 6, 12);
    }
     */
}


int gameLoop() {

    XClearWindow(d, w);
    XStoreName(d, w, "Generic game?!");

    struct Box boxes[5000];
    int firstFreeBox;
    int playerX = 500;
    int playerY = 400;
    struct timespec start, end;
    int keyDown = 0;
    int stopW = 0;
    int stopS = 0;
    int stopA = 0;
    int stopD = 0;

    //Read level from file and save to boxes[]
    FILE *f = fopen("level.data", "rb");
    fread(boxes, sizeof(struct Box), 5000, f);
    fclose(f);

    //get firstFreeBox from last element of array
    for (int i = 5000; i >= 0; --i) {
        if (boxes[i].x != 0) {
            firstFreeBox = boxes[i].x;
            break;
        }
    }

    while (1)       //Main loop
    {
        drawLevel(boxes, firstFreeBox);
        drawPlayer(playerX, playerY);

        XEvent e;
        XNextEvent(d, &e);
        printf("got event: %s\n", event_names[e.type]);     //Debug tool

        if (e.type == KeyPress) {
            clock_gettime(CLOCK_REALTIME, &start);
            keyDown = 1;
            char key = keyPressed(e);

            if (key == 27) {
                system("xset r on");
                XCloseDisplay(d);
                return 0;
            }
            if ((key == 'w') || (key == 's') || (key == 'a') || (key == 'd')) {

                while(XCheckIfEvent(d, &e, CheckIfKeyRelease, &key) == False) {

                    clock_gettime(CLOCK_REALTIME, &end);
                    double timeDiff = (end.tv_sec - start.tv_sec) + ((end.tv_nsec - start.tv_nsec) / BILLION);
                    printf("timediff = %f\n", timeDiff);

                    /*
                                * Bottom edge of box
                                *
                                *(box.x, box.y) to (box.x + WIDTH, box.y)
                                *
                                * playerX and PlayerY are from top left of player
                                * Box.x and box.y are from bottom left of box
                                *
                                * playerX, PlayerY
                                */

                    if (timeDiff > 0.005) {
                        if (key == 'w') {
                            stopW = 0;

                            for (int i = 0; i < firstFreeBox; i++) {
                                if (boxes[i].y == playerY &&
                                    boxes[i].x < (playerX + TEXTWIDTH * 3) &&
                                    (boxes[i].x + TEXTWIDTH) > playerX) {
                                    stopW = 1;     // HELP
                                }
                            }
                            if (stopW == 0)
                                playerY -= 1;
                        }

                        else if (key == 's') {
                            stopS = 0;

                            for (int i = 0; i < firstFreeBox; i++) {        //SFJK:LKJDSF:LKJSDF:LKJDSF
                                if ((boxes[i].y - TEXTHEIGHT) == (playerY + TEXTHEIGHT * 4) &&
                                    boxes[i].x < (playerX + TEXTWIDTH * 3) &&
                                    (boxes[i].x + TEXTWIDTH) > playerX) {
                                    stopS = 1;     // HELP
                                }
                            }
                            if (stopS == 0)
                                playerY += 1;
                        }

                        else if (key == 'a') {
                            stopA = 0;

                            for (int i = 0; i < firstFreeBox; i++) {
                                if ((boxes[i].x + TEXTWIDTH) == playerX &&
                                    boxes[i].y > playerY &&
                                    (boxes[i].y - TEXTHEIGHT) < (playerY + TEXTHEIGHT * 4)) {
                                    stopA = 1;     // HELP
                                }
                            }
                            if (stopA == 0)
                                playerX -= 1;
                        }


                        else if (key == 'd'){
                            stopD = 0;

                            for (int i = 0; i < firstFreeBox; i++) {
                                if (boxes[i].x == (playerX + TEXTWIDTH * 3) &&
                                    boxes[i].y > playerY &&
                                    (boxes[i].y - TEXTHEIGHT) < (playerY + TEXTHEIGHT * 4)) {
                                    stopD = 1;     // HELP
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
 * Functions need GC pointer to list, not single gc?
 *
 * DONE - Add drawing of correct colours
 *
 * DONE - add player sprite
 *
 * DONE - add movement - biggie - xcleararea can create event?
 *
 * DONE  - collision detection
 *
 * detect type on collision, teleport
 *
 * improve editor, allow for multiple rooms....
 *
 * brush size
 *
 * etc
 */