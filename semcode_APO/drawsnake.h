#ifndef DRAWSNAKE
#define DRAWSNAKE

#ifdef __cplusplus
extern "C" {
#endif

// fce zapíše do buffru jeden velký pixel hada
void draw_snake_part(int x, int y,unsigned short color, int size,unsigned short *fb);


// fce by měla v konbinaci s fcí draw_snake_part() vykreslit celého hada na displej
void draw_snake(int x1, int y1, int *buffer, unsigned short color, int size,unsigned short *fb);



#ifdef __cplusplus
} /* extern "C"*/
#endif

#endif
