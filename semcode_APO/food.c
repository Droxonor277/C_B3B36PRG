#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "food.h"
#

#define HEIGHT 320
#define WEIGHT 480

void draw_pix(int x, int y, unsigned int color, int size,unsigned short *fb){
    int tmp_x = x;
    int tmp_y = y;
    for(int i = 0; i < size; i++) {
        tmp_y = y;
        if (i > 0)
            tmp_x += 1;
        for(int j = 0; j < size; j++) {
            if ( j > 0)
                tmp_y += 1;

            // kontrola jestli je to v rozmezí displeye
            if(tmp_x >= 0 && tmp_x < WEIGHT && tmp_y >= 0 && tmp_y < HEIGHT){
                fb[tmp_x + (WEIGHT*tmp_y)] = color;
            } else {
                fprintf(stdout,"Pixel mňamky není to v rozsahu displeye\n");
                fflush(stdout);
            }
        }
    }    
}

bool check_feed(int x1, int y1, int pixx, int pixy, int size){
    bool ret = false;
    
    if (x1 == pixx && y1 == pixy){
        ret = true;
    }
    
    return ret;
}

