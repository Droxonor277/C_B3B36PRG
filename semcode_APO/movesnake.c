#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "drawsnake.h"

#define HEIGHT 320
#define WEIGHT 480

void update_coords(int *x, int *y, int buffer_number, int size){
    if ( buffer_number == 1) { // další část hada je nahoře od stávající části
        *y -= size;
        if (*y < 0){
            *y = (HEIGHT - size);
        }
    } else if (buffer_number == 2) { // další část hada je dole od stávající části
        *y += size;
        if (*y >= HEIGHT){
            *y = 0;
        }
    } else if (buffer_number == 3) { // další část hada je napravo od stávající části
        *x += size;
        if (*x >= WEIGHT){
            *x = 0;
        }
    } else if (buffer_number == 4) { // další část hada je nalevo od stávající části
        *x -= size;
        if (*x < 0){
            *x = (WEIGHT - size );
        }
    }
}

void coords_snake_one(int x1, int y1, int *buffer1, int size){
    int buffer_size = 5; // velikost bufru
    int tmp_x1 = x1; // tmp zkopírování proměnné x pro hada 
    int tmp_y1 = y1; // tmp zkopírování proměnná y pro hada

    fprintf(stdout,"%d - x_coord_head, %d - y_coord_head\n",x1,y1);
    for (int i = 1; i < buffer_size; i++ ){
        update_coords(&tmp_x1,&tmp_y1,buffer1[i], size);
        fprintf(stdout,"%d - x_coord_tail_part_%d, %d - y_coord_tail_part_%d\n", tmp_x1,i,tmp_y1,i);
        fflush(stdout);
    }
}

bool Check_right_pozition_pix(int x1, int y1, int *buffer1, int pixx, int pixy, int size){
    // bool ret = false; 
    // návratová kotrolní proměnná rozhodující, jeslti jsou všechny pixeli hada neshodují s pozicí   
    int buffer_size = 4; // velikost bufru
    int tmp_x1 = x1; // tmp zkopírování proměné x pro hada 
    int tmp_y1 = y1; // tmp zkopírování proměná y pro hada 
    
    if (x1 == pixx && y1 == pixy){
        //printf("Špatná pozice\n");
        return false;
    }
    for (int i = 1; i < buffer_size; i++ ){
        update_coords(&tmp_x1,&tmp_y1,buffer1[i], size);
        if (tmp_x1 == pixx && tmp_y1 == pixy){
            
            //printf("Špatná pozice\n");
            return false;
        }
    }
    // kontrola jestli je mňamka v rozmezí displeje
    if (pixx < 0 || pixx >= WEIGHT || pixy < 0 || pixy >= HEIGHT){
        return false;
    }
    return true;
}

void shift_move_buffer(int *buffer1){
    int buffer_size = 4;
    int tmp1 = 0; 
    // posun všech čísel v bufru o +1 a číslo na poslední pozici zahodit 
    for (int i = 0; i < buffer_size; i++) {
        tmp1 = buffer1[0];
        buffer1[0] = buffer1[i+1]; 
        buffer1[i+1] = tmp1;
    }
    // příprava první pozice pro zadání znaku od uživatele
    buffer1[0] = 0;

}

int move_down(int *y, int size){
    *y += size;
    if (*y >= HEIGHT){
        *y = 0;
    }
    return *y;
}

int move_up(int *y, int size){
    *y -= size;
    if (*y < 0){
        *y = (HEIGHT - size);
    }
    return *y;
}

int move_right(int *x, int size){
    *x += size;
    if (*x >= WEIGHT){
        *x = 0;
    }
    return *x;
}

int move_letf(int *x, int size){
    *x -= size;
    if (*x < 0){
        *x = (WEIGHT - size);
    }
    return *x;
}

bool Check_snake(int* buffer, int x1, int y1, int size){
    bool ret = false;
    int buffer_size = 5;
    int coord_x = x1;
    int coord_y = y1;

    for (int i = 1; i < buffer_size; i++ ){
        update_coords(&coord_x,&coord_y,buffer[i], size);

        if (x1 == coord_x && y1 == coord_y){
            ret = true;
            return ret;
        }
    }
    return ret;
}

bool snake_fight(int x1, int y1, int x2, int y2, int *buffer, int size){
    int buffer_size = 5;
    int coord_x = x2;
    int coord_y = y2;
    bool ret = false;

    if (x1 == x2 && y1 == y2){
        return true;
    }

    for (int i = 1; i < buffer_size; i++ ){
        update_coords(&coord_x,&coord_y,buffer[i], size);

        if (x1 == coord_x && y1 == coord_y){
            ret = true;
            return ret;
        }
    }
    return ret;
}
