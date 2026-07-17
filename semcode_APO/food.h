#ifndef FOOD
#define FOOD

#ifdef __cplusplus
extern "C" {
#endif

// fce zapíše do buffru mňamku
void draw_pix(int x, int y, unsigned int color, int size,unsigned short *fb);

// fce vrací true pokud had mňamku sežral
bool check_feed(int x1, int y1, int pixx, int pixy, int size);

#ifdef __cplusplus
} /* extern "C"*/
#endif

#endif
