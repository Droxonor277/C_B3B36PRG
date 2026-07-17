#ifndef MENU  
#define MENU

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

//ukládá barvu do bufferu
void draw_pixel(int x, int y, unsigned short color,unsigned short *fb);

//vykreslí char 
void draw_char(int x, int y, char ch, int scale, unsigned short color,unsigned short *fb);

//vykreslí menu
void draw_menu(unsigned int color_chars,unsigned short *fb);

//vykreslí end pro výhru hada 1
void draw_end1(unsigned int color_chars,unsigned short *fb);

//vykreslí end pro výhru hada 1
void draw_end2(unsigned int color_chars,unsigned short *fb);

//kreslí pixely zvětšení o scale
void draw_pixel_big(int x, int y, int scale, unsigned short color,unsigned short *fb);

//vrací hodnotu šířky
int char_width(int ch);

#ifdef __cplusplus
} /* extern "C"*/
#endif

#endif
