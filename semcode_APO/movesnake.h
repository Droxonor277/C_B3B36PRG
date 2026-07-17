#ifndef MOVESNAKE
#define MOVESNAKE

#ifdef __cplusplus
extern "C" {
#endif

// aktualizuje souřadnice hada při jeho přocházení po kontrolních souřadnicích každého pixelu
void update_coords(int *x, int *y, int buffer_number, int size);

void coords_snake_one(int x1, int y1, int *buffer1, int size);

// fce vrací true pokud se mňamka neshoduje s velkými pixeli jednoho hada 
bool Check_right_pozition_pix(int x1, int y1, int *buffer1, int pixx, int pixy, int size);

// fce bude posouvat buffer o +1, poslední číslo bude zahazovat, první pozici bude 
// připravovat na vstup uřivatele nebo pro další nastavený postup hada
void shift_move_buffer(int *buffer1);

// fce vrací hodnotu y posunutou dolu
int move_down(int *y, int size);

// fce vrací hodnotu y posunutou nahoru
int move_up(int *y, int size);

// fce vrací hodnotu x posunutou doprava
int move_right(int *x, int size);

// fce vrací hodnotu x posunutou doleva
int move_letf(int *x, int size);

// fce vrací true pokud sám do sebe had narazil
bool Check_snake(int* buffer, int x1, int y1, int size);

// fce vrací false pokud had nenarazil do druhého hada v opačném případě vrací true 
bool snake_fight(int x1, int y1, int x2, int y2, int *buffer, int size);



#ifdef __cplusplus
} /* extern "C"*/
#endif

#endif
