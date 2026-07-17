#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "menu.h"

#include "font_types.h"


font_descriptor_t *fdes; // struktura fontů
//fdes = &font_winFreeSystem14x16; // definování fontu

void draw_pixel(int x, int y, unsigned short color, unsigned short *fb) {
  if (x>=0 && x<480 && y>=0 && y<320) {
    fb[x+480*y] = color;
  }
}

int char_width(int ch) {
  int width;
  fdes = &font_winFreeSystem14x16;
  if (!fdes->width) {
    width = fdes->maxwidth;
  } else {
    width = fdes->width[ch-fdes->firstchar];
  }
  return width;
}

void draw_pixel_big(int x, int y, int scale, unsigned short color,unsigned short *fb) {
  int i,j;
  for (i=0; i<scale; i++) {
    for (j=0; j<scale; j++) {
      draw_pixel(x+i, y+j, color,fb);
    }
  }
}

void draw_char(int x, int y, char ch, int scale, unsigned short color,unsigned short *fb) {
  int w = char_width(ch); //šířka písmenka
  const font_bits_t *ptr;
  fdes = &font_winFreeSystem14x16;
  if ((ch >= fdes->firstchar) && (ch-fdes->firstchar < fdes->size)) {
    if (fdes->offset) {
      ptr = &fdes->bits[fdes->offset[ch-fdes->firstchar]];
    } else {
      int bw = (fdes->maxwidth+15)/16;
      ptr = &fdes->bits[(ch-fdes->firstchar)*bw*fdes->height];
    }
    int i, j;
    for (i=0; i<fdes->height; i++) {
      font_bits_t val = *ptr;
      for (j=0; j<w; j++) {
        if ((val&0x8000)!=0) {
          draw_pixel_big(x+scale*j, y+scale*i, scale, color,fb);
        }
        val<<=1;
      }
      ptr++;
    }
  }
}

void draw_menu(unsigned int color_chars,unsigned short *fb){
        int scale1 = 4; //zvětšení vykreslování pixelů u nadpisu
        int scale2 = 3; //zvětšení vykreslování textu "Press 'P' "
        int x1 = 67;
        int x2 = 71;
        int y1 = 99;
        int y2 = 171;

        draw_char(x1, y1, 'S',scale1, color_chars,fb);
        x1 += scale1 * 9; // 9 - šířka písmene S 
        draw_char(x1, y1, 'T', scale1, color_chars,fb);
        x1 += scale1 * 8; // 8 - šířka písmene T
        draw_char(x1, y1, 'A', scale1, color_chars,fb);
        x1 += scale1 * 8; // 8 - šířka písmene A
        draw_char(x1, y1, 'R', scale1,color_chars,fb);
        x1 += scale1 * 10; // 10 - šířka písmene R
        draw_char(x1, y1, 'T', scale1, color_chars,fb);
        x1 += scale1 * 8; // 8 - šířka písmene T
        x1 += scale1 * 4; // 4 - šířka mezery
        draw_char(x1, y1, 'G', scale1, color_chars,fb);
        x1 += scale1 * 10; // 10 - šířka písmene G
        draw_char(x1, y1, 'A', scale1, color_chars,fb);
        x1 += scale1 * 8; // 8 - šířka písmene A
        draw_char(x1, y1, 'M', scale1, color_chars,fb);
        x1 += scale1 * 12; // 12 - šířka písmene M
        draw_char(x1, y1, 'E', scale1, color_chars,fb);

        draw_char(x2, y2, 'P', scale2, color_chars,fb);
        x2 += scale2 * 9; // 9 - šířka písmene P
        draw_char(x2, y2, 'r',  scale2, color_chars,fb);
        x2 += scale2 * 5; // 5 - šířka písmene r
        draw_char(x2, y2, 'e',  scale2, color_chars,fb);
        x2 += scale2 * 8; // 8 - šířka písmene e
        draw_char(x2, y2, 's',  scale2,color_chars,fb);
        x2 += scale2 * 8; // 8 - šířka písmene s
        draw_char(x2, y2, 's',  scale2, color_chars,fb);
        x2 += scale2 * 8; // 8 - šířka písmene s
        x2 += scale2 * 4; // 4 - šířka mezery
        draw_char(x2, y2, '\'',  scale2, color_chars,fb);
        x2 += scale2 * 4; // 4 - šířka uvozovky
        draw_char(x2, y2, 'P',  scale2, color_chars,fb);
        x2 += scale2 * 9; // 9 - šířka písmene P
        draw_char(x2, y2, '\'',  scale2, color_chars,fb);
    }

void draw_end1(unsigned int color_chars,unsigned short *fb){
        int scale1 = 4; //zvětšení vykreslování pixelů u nadpisu
        int scale2 = 3; //zvětšení vykreslování textu "Press 'Q' "
        int x1 = 67;
        int x2 = 69;
        int y1 = 99;
        int y2 = 171;

        draw_char(x1, y1, 'W',scale1, color_chars,fb);
        x1 += scale1 * 14; // 14 - šířka písmene W 
        draw_char(x1, y1, 'I', scale1, color_chars,fb);
        x1 += scale1 * 4; // 4 - šířka písmene I
        draw_char(x1, y1, 'N', scale1, color_chars,fb);
        x1 += scale1 * 10; // 10 - šířka písmene N
        draw_char(x1, y1, 'N', scale1,color_chars,fb);
        x1 += scale1 * 10; // 10 - šířka písmene N
        draw_char(x1, y1, 'E', scale1, color_chars,fb);
        x1 += scale1 * 9; // 9 - šířka písmene E
        draw_char(x1, y1, 'R', scale1, color_chars,fb);
        x1 += scale1 * 10; // 10 - šířka písmene R
        x1 += scale1 * 4; // 4 - šířka mezery
        draw_char(x1, y1, 'I', scale1, color_chars,fb);
        x1 += scale1 * 4; // 4 - šířka písmene I
        draw_char(x1, y1, 'S', scale1, color_chars,fb);
        x1 += scale1 * 9; // 9 - šířka písmene S
        x1 += scale1 * 4; // 4 - šířka mezery
      
        draw_char(x1, y1, '1', scale1, color_chars,fb);
        //draw_char(x1, y1, '2', scale1, color_chars,fb);

        draw_char(x2, y2, 'P', scale2, color_chars,fb);
        x2 += scale2 * 9; // 9 - šířka písmene P
        draw_char(x2, y2, 'r',  scale2, color_chars,fb);
        x2 += scale2 * 5; // 5 - šířka písmene r
        draw_char(x2, y2, 'e',  scale2, color_chars,fb);
        x2 += scale2 * 8; // 8 - šířka písmene e
        draw_char(x2, y2, 's',  scale2,color_chars,fb);
        x2 += scale2 * 8; // 8 - šířka písmene s
        draw_char(x2, y2, 's',  scale2, color_chars,fb);
        x2 += scale2 * 8; // 8 - šířka písmene s
        x2 += scale2 * 4; // 4 - šířka mezery
        draw_char(x2, y2, '\'',  scale2, color_chars,fb);
        x2 += scale2 * 4; // 4 - šířka uvozovky
        draw_char(x2, y2, 'Q',  scale2, color_chars,fb);
        x2 += scale2 * 9; // 9 - šířka písmene Q
        draw_char(x2, y2, '\'',  scale2, color_chars,fb);
    }

void draw_end2(unsigned int color_chars,unsigned short *fb){
        int scale1 = 4; //zvětšení vykreslování pixelů u nadpisu
        int scale2 = 3; //zvětšení vykreslování textu "Press 'Q' "
        int x1 = 67;
        int x2 = 69;
        int y1 = 99;
        int y2 = 171;

        draw_char(x1, y1, 'W',scale1, color_chars,fb);
        x1 += scale1 * 14; // 14 - šířka písmene W 
        draw_char(x1, y1, 'I', scale1, color_chars,fb);
        x1 += scale1 * 4; // 4 - šířka písmene I
        draw_char(x1, y1, 'N', scale1, color_chars,fb);
        x1 += scale1 * 10; // 10 - šířka písmene N
        draw_char(x1, y1, 'N', scale1,color_chars,fb);
        x1 += scale1 * 10; // 10 - šířka písmene N
        draw_char(x1, y1, 'E', scale1, color_chars,fb);
        x1 += scale1 * 9; // 9 - šířka písmene E
        draw_char(x1, y1, 'R', scale1, color_chars,fb);
        x1 += scale1 * 10; // 10 - šířka písmene R
        x1 += scale1 * 4; // 4 - šířka mezery
        draw_char(x1, y1, 'I', scale1, color_chars,fb);
        x1 += scale1 * 4; // 4 - šířka písmene I
        draw_char(x1, y1, 'S', scale1, color_chars,fb);
        x1 += scale1 * 9; // 9 - šířka písmene S
        x1 += scale1 * 4; // 4 - šířka mezery
      
        //draw_char(x1, y1, '1', scale1, color_chars,fb);
        draw_char(x1, y1, '2', scale1, color_chars,fb);

        draw_char(x2, y2, 'P', scale2, color_chars,fb);
        x2 += scale2 * 9; // 9 - šířka písmene P
        draw_char(x2, y2, 'r',  scale2, color_chars,fb);
        x2 += scale2 * 5; // 5 - šířka písmene r
        draw_char(x2, y2, 'e',  scale2, color_chars,fb);
        x2 += scale2 * 8; // 8 - šířka písmene e
        draw_char(x2, y2, 's',  scale2,color_chars,fb);
        x2 += scale2 * 8; // 8 - šířka písmene s
        draw_char(x2, y2, 's',  scale2, color_chars,fb);
        x2 += scale2 * 8; // 8 - šířka písmene s
        x2 += scale2 * 4; // 4 - šířka mezery
        draw_char(x2, y2, '\'',  scale2, color_chars,fb);
        x2 += scale2 * 4; // 4 - šířka uvozovky
        draw_char(x2, y2, 'Q',  scale2, color_chars,fb);
        x2 += scale2 * 9; // 9 - šířka písmene Q
        draw_char(x2, y2, '\'',  scale2, color_chars,fb);
    }
