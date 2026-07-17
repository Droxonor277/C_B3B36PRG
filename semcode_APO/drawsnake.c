#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "drawsnake.h"
#include "movesnake.h"

#define HEIGHT 320
#define WEIGHT 480

void draw_snake_part(int x, int y,unsigned short color, int size, unsigned short *fb){
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
                fb[tmp_x+(WEIGHT*tmp_y)] = color;
            } else {
                fprintf(stdout,"Pixel hada není to v rozsahu displeye\n");
                fflush(stdout);
            }
        }
    }
}



void draw_snake(int x1, int y1, int *buffer, unsigned short color, int size, unsigned short *fb){

    int buffer_size = 5; // velikost buffru
    int buffer_number = 0; // proměná na uložení čísla z bufru
    int tmp_x = x1; // překopírování x-ové souřadnice hlavy hada
    int tmp_y = y1; // překopírování y-ové souřadnice hlavy hada

    // vykreslí hlavu hada
    draw_snake_part( x1, y1,color, size,fb);
    //printf(" %d - x, %d - y\n", tmp_x,tmp_y);

    // vykreslí zbytek hada
    for(int i = 1; i < buffer_size; i++) {
        buffer_number = buffer[i];
        update_coords(&tmp_x, &tmp_y, buffer_number, size);
        draw_snake_part(tmp_x, tmp_y, color, size,fb);
        //printf(" %d - x, %d - y\n", tmp_x,tmp_y);
    } 

}

