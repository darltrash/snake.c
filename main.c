/*
    Made by Nelson "darltrash" Lopez

    Feel free to do whatever you want with this code.
*/

#define _POSIX_C_SOURCE 199309L
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <pthread.h>
#include <X11/Xlib.h>

Display *display;
Window window;
XEvent event;
int screen;

// CALLBACKS /////////////////////////////////////////////////////

#define WIDTH 30
#define HEIGHT 30
#define GRID_SIZE 10

enum {
    BLOCK_NONE = 0,
    BLOCK_SNAKE_PART = 1,
    BLOCK_FOOD = 2
};

unsigned char snake_lifespans[WIDTH*HEIGHT];
unsigned char grid[WIDTH*HEIGHT];
int snake_position[2];
char snake_velocity[2] = {0, -1};

int clear = 0;
int process = 1;
int ticks = 0;
unsigned int score;

void init() {
    srand(time(NULL));

    memset(&snake_velocity, 0, sizeof(snake_velocity));
    memset(&grid, BLOCK_NONE, sizeof(grid));
    memset(&snake_lifespans, 0, sizeof(snake_lifespans));

    snake_position[0] = WIDTH  / 2;
    snake_position[1] = HEIGHT / 2;
    int i = snake_position[1] * WIDTH + snake_position[0];
    //grid[i] = BLOCK_SNAKE_PART;
    score = 0;
    process = 0;
    clear = 1;
}

void frame() {
    if (clear)
        XClearWindow(display, window);

    clear = 0;

    int moving = snake_velocity[0] != 0 || snake_velocity[1] != 0;

    if (process && moving) {
        ticks += 1;
        if (ticks == 10) {
            ticks = 0;

            int i = rand() % (WIDTH * HEIGHT);

            while (grid[i] != BLOCK_NONE) 
                i = rand() % (WIDTH * HEIGHT);

            grid[i] = BLOCK_FOOD;
        }

        process = 0;

        snake_position[0] += snake_velocity[0];
        snake_position[1] += snake_velocity[1];

        int i = snake_position[1] * WIDTH + snake_position[0];

        if (
            (snake_position[0] < 0     || snake_position[1] < 0) ||
            (snake_position[0] > WIDTH || snake_position[1] > HEIGHT) ||
            (grid[i] == BLOCK_SNAKE_PART)
        ) {
            printf("Lost! \n");
            return init();
        } else if (grid[i] == BLOCK_FOOD) {
            score += 1;
            clear = 1;
        }

        // should be p easy to cache
        for (int p = 0; p < WIDTH * HEIGHT; p++) {
            if (snake_lifespans[p] == 0) continue;

            snake_lifespans[p] -= 1;
            if (snake_lifespans[p] <= 0) {
                snake_lifespans[p] = 0;
                grid[p] = BLOCK_NONE;
            }
        }

        grid[i] = BLOCK_SNAKE_PART;
        snake_lifespans[i] = score + 1;
    }

    GC gc = DefaultGC(display, screen);

    XSetForeground(display, gc, 0);

    XWindowAttributes attributes;
    XGetWindowAttributes(display, window, &attributes);

    int x = (attributes.width /2) - (GRID_SIZE * WIDTH ) / 2;
    int y = (attributes.height/2) - (GRID_SIZE * HEIGHT) / 2;

    XDrawRectangle(
        display, window, gc, 
        x-1, y-1,
        (GRID_SIZE * WIDTH )+2, 
        (GRID_SIZE * HEIGHT)+2
    );

    unsigned char buffer[30];
    snprintf(buffer, 30, "Score: %i", score);

    XTextItem text = {
        .chars = buffer,
        .nchars = strlen(buffer)
    };
    XDrawText(display, window, gc, x, y-8, &text, 1);

    for (int i=0; i < WIDTH * HEIGHT; i++) {
        int e = grid[i];
        
        XSetForeground(display, gc, 
            (e == BLOCK_NONE) ? 16777215 : 
            (e == BLOCK_FOOD) ? 16711680 :
            0
        );

        XFillRectangle(
            display, window, gc, 
            x + (GRID_SIZE * (i % WIDTH)), 
            y + (GRID_SIZE * floor((float)(i/HEIGHT))), 
            GRID_SIZE, GRID_SIZE
        );
    }
}

// MAIN LOOP /////////////////////////////////////////////////////

void* await(void* a) {
    XEvent exppp;

    while (1) {
        usleep(300000); // GOD, I HATE THIS
        memset(&exppp, 0, sizeof(exppp));
        exppp.type = Expose;
        exppp.xexpose.window = window;
        process = 1;
        XSendEvent(display, window, 0, ExposureMask, &exppp);
        XFlush(display);
    }
    return NULL;
}

int main() {
    pthread_t thread;
    pthread_create(&thread, NULL, await, NULL);
    
    display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Cannot open display\n");
        exit(1);
    }

    screen = DefaultScreen(display);

    window = XCreateSimpleWindow(display, 
        RootWindow(display, screen), 20, 20, 400, 400, 1, 
        BlackPixel(display, screen), WhitePixel(display, screen)
    );

    Atom del_window = XInternAtom(display, "WM_DELETE_WINDOW", 0);
    XSetWMProtocols(display, window, &del_window, 1);

    XSelectInput(display, window, ExposureMask | KeyPressMask);
    XMapWindow(display, window);

    init();

    while (1) {
        XNextEvent(display, &event);

        switch (event.type) {
            case KeyPress: {
                switch ( event.xkey.keycode ) {
                    case 111: { // UP
                        snake_velocity[0] = 0;
                        snake_velocity[1] = -1;
                        break;
                    }

                    case 116: { // DOWN
                        snake_velocity[0] = 0;
                        snake_velocity[1] = 1;
                        break;
                    }

                    case 113: { // LEFT
                        snake_velocity[0] = -1;
                        snake_velocity[1] = 0;
                        break;
                    }

                    case 114: { // RIGHT
                        snake_velocity[0] = 1;
                        snake_velocity[1] = 0;
                        break;
                    }

                    case 0x09:  // ESC
                        goto breakout;
                }
                break;
            }

            case ClientMessage: goto breakout;
            case Expose: frame();
        }
    }
breakout:

    XDestroyWindow(display, window);
    XCloseDisplay(display);

    return 0;
}