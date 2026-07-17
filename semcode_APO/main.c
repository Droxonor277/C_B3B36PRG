#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <termios.h> 

#include "mzapo_parlcd.h"
#include "mzapo_phys.h"
#include "mzapo_regs.h"
#include "font_types.h"

#define HEIGHT 320
#define WEIGHT 480
#define MAXIMUM_POINTS_TO_WIN 16

// vlákno přijímající znaky od uživatele a distribujuje  
void* InputThread(void*);

// vlánko ovládající led diodu
void* Control_LED_DIOD(void*);

// vlánko ovládající displej
void* Control_Displej(void*);

// vlákno na ovládání led řádku
void* Control_LED_CHAIN(void*);

unsigned short *fb; // inicializace adresy pro uložení displeye
int size = 20; // velikost jednoho pixelu hada
int scale=4;


#include "drawsnake.h"
#include "food.h"
#include "menu.h"
#include "movesnake.h"

void info_control_key(void)
{
    fprintf(stdout,"Ovládací klíče:\n"
            "Spuštění hry: 'p'\n"
            "Ukonení programu: 'q'\n"
            "Informace o souřadnicích hada: 'x'\n"
            "----- INFORMACE O OVLÁDÁNÍ HADA 1 -----\n"
            "Přikázat hadovi jít nahoru hadovi 1: 'w'\n"
            "Přikázat hadovi jít dolu hadovi 1: 's'\n"
            "Přikázat hadovi jít doprava hadovi 1: 'd'\n"
            "Přikázat hadovi jít doleva hadovi 1: 'a'\n"
            "----- INFORMACE O OVLÁDÁNÍ HADA 2 -----\n"
            "Přikázat hadovi jít nahoru hadovi 2: 'u'\n"
            "Přikázat hadovi jít dolu hadovi 2: 'j'\n"
            "Přikázat hadovi jít doprava hadovi 2: 'k'\n"
            "Přikázat hadovi jít doleva hadovi 2: 'h'\n");            
    fflush(stdout);
}

// funkce spouštějící raw mod terminálu
void call_termios(int reset)
{
   static struct termios tio, tioOld;
   tcgetattr(STDIN_FILENO, &tio);
   if (reset) {
      tcsetattr(STDIN_FILENO, TCSANOW, &tioOld);
   } else {
      tioOld = tio; //backup 
      cfmakeraw(&tio);
      tio.c_oflag |= OPOST;
      tcsetattr(STDIN_FILENO, TCSANOW, &tio);
   }
}

// struktura na sdílení dat
typedef struct 
{
    bool quit; // kontrolní proměná na ukončení proměnu
    bool pauze; //kontrolní proměnná pro spuštění hada
    int counter; // tmp počítadlo  - asi nepotřebná
    int tmp; // nějaká tmp proměná int - asi nepotřebná
    int tmp2; // proměná k předávání instrukcí hadovi od uživatele pro hada 2
    int player_one_points; // počet bodů hada 1
    int player_two_points; // pořet bodů hada 2
    bool win_one_player; // kontrolní proměná výhry 1
    bool win_two_player; // kontrolná proměná výhry 2
    bool coords_snake; // kontrolní proměná pro zjištění souřadnic hada 1
    bool snake_feed; // kontrolní promění na zjištění jestli se najedl had
    bool dead_snake; // kontrolní promění na zjištění, jeslti had umřel vlastní chybou
} SharedData;

// inicializace dat
SharedData initDATA()
{
    SharedData ret;
    ret.quit = false;
    ret.pauze = false;
    ret.counter = 0;
    ret.tmp = 0;
    ret.tmp2 = 0;
    ret.player_one_points = 0;
    ret.player_two_points = 0;
    ret.win_one_player = false;
    ret.win_two_player = false;
    ret.coords_snake = false;
    ret.snake_feed = false;
    ret.dead_snake = false;
    return ret;
}

// Zadefinování potřebných fcí pro ovládání vláken
pthread_mutex_t mtx;
pthread_cond_t condvar;

int main()
{
    SharedData data = initDATA(); // Dání dat do programu

    info_control_key(); //informování uřivatele o možnostech ovládání 

    // zadefinování fcí na ochranu dat ve vláknu a fce na vysílání signálu mezi vlákny
    pthread_mutex_init(&mtx, NULL);
    pthread_cond_init(&condvar, NULL);

    // zadefinování počtu a obecně zadefinování vláken použitých v budoucnosti
    enum {INPUT, CONTROL_LED_DIOD, CONTROL_DISPLEJ, CONTROL_LED_DIOD_CHAIN,NUM_THREADS };
    void* (*thr_threads[])(void*) = {InputThread, Control_LED_DIOD,Control_Displej,Control_LED_CHAIN};

    call_termios(0); // spuštění raw režimu

    // Vytvoření Vláken
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_create(&threads[i], NULL, thr_threads[i], &data);
    // Koretní ukončení všech vláken  
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }    

    call_termios(1); // ukončení raw režimu

    // zpráva o ukončení hlavího vlákna
    fprintf(stdout, "MAIN thread was closed!\n");
    fflush(stdout);
    free(fb);


    return 0;
}

void* Control_Displej(void* v)
{
    // zadefinování společních dat do vlákna
    SharedData* data = (SharedData*) v;

    unsigned char *parlcd_mem_base; // proměná pro uložení fyzické adresy

    bool q = false; // kontrolní proměná na ukončení průběhu vlánka
    bool pauze = false; // kontrolní proměnná pro spuštění hada

    // předání jednotlivých proměných z globálních data
    pthread_mutex_lock(&mtx);
    int point_one = data->player_one_points;
    int point_two = data->player_two_points;
    pthread_mutex_unlock(&mtx);

    int bufer1[5] = {1,1,1,1,1}; // buffer pro sanke 1
    int bufer2[5] = {1,1,1,1,1}; // buffer pro snake 2

    int x1 = 0; // x-ová souřadnice hlavy hada 1
    int y1 = 80; // y-ová souřadnice hlavy hada 1

    int x2 = 460; // x-ová souřadnice hlavy hada 2
    int y2 = 80; // y-ová souřadnice hlavy hada 2

    int pixx = 0; // x-ová souřadnice mňamky
    int pixy = 0;//10; // y-ová souřadnice mňamky

    bool Check_food = false; // kontrolní proměná detekující, jestli bylo jídlo snědeno nebo ne
    bool quit_search_pix = true; // kontrolní proměná na hledání mňamky pro hada

    // kontrolní proměné na znemožní ovládání hada pomocí ovládacími klíčemi v jednotlivých směrech
    bool stay_way_down = true;
    bool stay_way_up = false;
    bool stay_way_right = false;
    bool stay_way_left = false;

    // kontrolní proměné na znemožní ovládání hada 2 pomocích ovládacích klíčů v jednotlivých směrech
    bool stay_way_right2 = false;
    bool stay_way_up2 = false;
    bool stay_way_left2 = false;
    bool stay_way_down2 = true;
    
    int index = 0; // index pro pohyb v bufferu

    fb  = (unsigned short *)malloc(HEIGHT*WEIGHT*2); // alokace paměti pro zapis do pole

    // výber jednotlivých barev 
    unsigned int color1 = (((255 & 0x1f) << 11) |((0&0x3f)<<5)|(0&0x1f)); // barva hada jedna
    unsigned int color2 = (((0 & 0x1f) << 11) |((255 & 0x3f)<<5)|(0 & 0x1f)); // barva hada dva
    unsigned int color_pix = (((255 & 0x1f) << 11) |((255 & 0x3f)<<5)|(0 & 0x1f)); // barva potravi pro hady
    unsigned int color_chars = (((255&0x1f)<<11)|((255&0x3f)<<5)|(255&0x1f)); //barva písmenek  

    int counter = 0; // počítadlo cyklů

    // namapování fyzické adresy displeye
    parlcd_mem_base = map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0); 

    parlcd_hx8357_init(parlcd_mem_base); // inicializace displeye

    int ptr = 0; // proměná pro pohyb v buffru

    // reset obrazoky na černou obrazovku
    parlcd_write_cmd(parlcd_mem_base, 0x2c);
    for (int i = 0; i < 320 ; i++) {
        for (int j = 0; j < 480 ; j++) {
        fb[ptr]=0;
        parlcd_write_data(parlcd_mem_base, fb[ptr++]);
        }
    }

    // reset bufferu
    for (ptr = 0; ptr < 320*480 ; ptr++) {
    fb[ptr]=0;
    }

    draw_menu(color_chars,fb); //vykreslení menu do buffru

    // vykreslení startovacího meny na obrazovku
    parlcd_write_cmd(parlcd_mem_base, 0x2c);
    for (ptr = 0; ptr < 320*480; ptr++){
             parlcd_write_data(parlcd_mem_base, fb[ptr]);
        }

    // pozastavení chodu hada, čekající na kontrolní znak od uživatele
    while(!pauze){
        pthread_mutex_lock(&mtx);
        pauze = data->pauze;
        pthread_mutex_unlock(&mtx);
    }

    while (!q && (point_one <= MAXIMUM_POINTS_TO_WIN && point_two <= MAXIMUM_POINTS_TO_WIN ))
    {
        // vypsání souřadnic hada 
        pthread_mutex_lock(&mtx);
        if (data->coords_snake)
        {
            coords_snake_one(x1,y1,bufer1,size);
            data->coords_snake = false;
        }
        pthread_mutex_unlock(&mtx);

        counter++;// počítadlo 

        // fce na reset buffru na černo
        for(index = 0; index < 320*480; index++) {
            fb[index] = 0;
        }

        // fce na posunutí bufferu
        shift_move_buffer(bufer1); // posunutí buferu 1
        shift_move_buffer(bufer2); // posunutí buferu 2     

        // vzpsání informací o hadovi
        printf("%d - x1, %d - y1\n",x1,y1);
        printf("%d - x2, %d - y1\n",x2,y2);

        // hledání nové mňamky pro hada
        if (!Check_food)
        {
            while (quit_search_pix)
            {
            
                srand((unsigned int)time(NULL));
                if (counter == 1)
                {
                    pixx = 240; // random vygenerování x-ové souřadnice
                    pixy = 160; // random vygenerování y-ové souřadnice
                } else {

                    pixx =  20*(rand() % 24); // random vygenerování x-ové souřadnice
                    pixy = 20*(rand() % 20); // random vygenerování y-ové souřadnice
                }

                // kontrola jestli se souřadnice mňamky neshodují s nějakou souřadnicí části těla hadů
                bool part1 = Check_right_pozition_pix(x1,y1,bufer1,pixx,pixy, size);
                bool part2 = Check_right_pozition_pix(x2,y2,bufer2,pixx,pixy,size);

                // kontrola jestli je možno ukončit hledání pozice nové mňamky po startu nebo sežrání
                if (part1 && part2)
                {
                    printf("Místo je volné\n");
                    printf("%d - pixx, %d - pixy\n", pixx, pixy); 
                    draw_pix(pixx,pixy,color_pix,size,fb); // zapsání mňamky do bufferu          
                    quit_search_pix = false;
                }
            } // end find snake_feed

            // reset hodnot pro budoucí hledání pixelů
            quit_search_pix = true;
            Check_food = true;
        } // end creating mňaky

        // pro opakované vypsání mňamky do bufru 
        if (Check_food)
        {
            draw_pix(pixx,pixy,color_pix,size,fb); // zapsání mňamky do bufferu
        }

        // kontrola jeslti hadi nenarazil každý sám do sebe
        if (Check_snake(bufer1,x1,y1,size))
        {
            fprintf(stdout,"Had 1 narazil sám do sebe\n");
            fflush(stdout);
            pthread_mutex_lock(&mtx);
            data->win_two_player = true; // zajištění výhry prvního hráče 
            q = data->quit = true;
            data->dead_snake = true;
            pthread_mutex_unlock(&mtx);
        }
        if (Check_snake(bufer2,x2,y2,size))
        {
            fprintf(stdout,"Had 2 narazil sám do sebe\n");
            fflush(stdout);
            pthread_mutex_lock(&mtx);
            data->win_one_player = true; // zajištění výhry druhého hráče
            q = data->quit = true;
            data->dead_snake = true; 
            pthread_mutex_unlock(&mtx);
            
        }

        // kontrola, jeslti jeden had do druhého narazil
        if (snake_fight(x1,y1,x2,y2,bufer2,size))
        {
            fprintf(stdout,"Had 1 narazil do Hada 2\n");
            fflush(stdout);
            pthread_mutex_lock(&mtx);
            data->win_two_player = true; // zajištění výhry druhého hráče
            q = data->quit = true;
            data->dead_snake = true; 
            pthread_mutex_unlock(&mtx);
            coords_snake_one(x2,y2,bufer2,size);
        }
        if (snake_fight(x2,y2,x1,y1,bufer1,size) && !snake_fight(x1,y1,x2,y2,bufer2,size))
        {
            fprintf(stdout,"Had 2 narazil do Hada 1\n");
            fflush(stdout);
            pthread_mutex_lock(&mtx);
            data->win_one_player = true; // zajištění výhry druhého hráče
            q = data->quit = true;
            data->dead_snake = true; 
            pthread_mutex_unlock(&mtx);
            coords_snake_one(x1,y1,bufer1,size);
        }

        // vykreslení hada do bufferu 
        draw_snake(x1,y1,bufer1,color1,size,fb);
        draw_snake(x2,y2,bufer2,color2,size,fb);

        // pokud-li uživatel nezadá nějakou změnu pohybu, 
        // tak had pokračuje v pohybu v posledním nebo nativně nastaveném pohybu
        pthread_mutex_lock(&mtx);
        if (data->tmp == 0)
        {
            // resetování blokace pohybu
            stay_way_right = stay_way_up = stay_way_left = stay_way_down = false;
            
            // Pokračující pohyb v jednom určitém směru bez zásahu uživatele
            if (bufer1[1] == 1)
            {
                move_down(&y1, size);
                stay_way_down = true;
            } else if (bufer1[1] == 2) {
                move_up(&y1,size);
                stay_way_up = true;
            } else if (bufer1[1] == 3) {
                move_letf(&x1,size);
                stay_way_right = true;
            } else if (bufer1[1] == 4) {
                move_right(&x1,size);
                stay_way_left = true;
            }
            bufer1[0] = bufer1[1];
        }
        pthread_mutex_unlock(&mtx);

        // ovládání hada
        pthread_mutex_lock(&mtx);
        if (data->tmp == 1) { // pohyb hada doleva
            fprintf(stdout, "LEFT\n");
            fflush(stdout);

            if (!stay_way_left)
            {
                move_letf(&x1,size);
                bufer1[0] = 3;
            } else {
                move_right(&x1,size);
                bufer1[0] = bufer1[1];
            }

            // reset předávané hodnoty
            data->tmp = 0;
        } else if (data->tmp == 2) { // pohyb hada nahoru
            fprintf(stdout, "UP\n");
            fflush(stdout);
            
            if (!stay_way_down)
            {
                move_up(&y1,size);
                bufer1[0] = 2;
            } else {
                bufer1[0] = bufer1[1];
                move_down(&y1,size);
            }

            // reset předávané hodnoty
            data->tmp = 0;
        } else if (data->tmp == 3) { // pohyb hada dolu
            fprintf(stdout, "DOWN\n");
            fflush(stdout);

            if (!stay_way_up)
            {
                move_down(&y1,size);
                bufer1[0] = 1;
            } else {
                move_up(&y1,size);
                bufer1[0] = bufer1[1];
            }

            // reset předávané hodnoty
            data->tmp = 0;            
        } else if (data->tmp == 4) { // pohyb hada doprava
            fprintf(stdout, "RIGHT\n");
            fflush(stdout);

            if (!stay_way_right)
            {
                move_right(&x1,size);
                bufer1[0] = 4;
            } else {
                move_letf(&x1,size);
                bufer1[0] = bufer1[1];
            }

            // reset předávané hodnoty
            data->tmp = 0;            
        }
        pthread_mutex_unlock(&mtx);

        // operace pro druhého hada - zatím nastavená, aby pokračovala furt rovně
        pthread_mutex_lock(&mtx);
        if (data->tmp2 == 0)
        {
            // resetování blokace pohybu
            stay_way_right2 = stay_way_up2 = stay_way_left2 = stay_way_down2 = false;

            // Pokračující pohyb v jednom určitém směru bez zásahu uživatele
            if (bufer2[1] == 1)
            {
                move_down(&y2, size);
                
                stay_way_down2 = true;
            } else if (bufer2[1] == 2) {
                move_up(&y2,size);
                stay_way_up2 = true;
            } else if (bufer2[1] == 3) {
                move_letf(&x2,size);
                stay_way_right2 = true;
            } else if (bufer2[1] == 4) {
                move_right(&x2,size);
                stay_way_left2 = true;
            }
            bufer2[0] = bufer2[1];
        }
        pthread_mutex_unlock(&mtx);

        // ovládání hada 2
        pthread_mutex_lock(&mtx);
        if (data->tmp2 == 1) { //  pohyb hada doleva
            fprintf(stdout, "LEFT\n");
            fflush(stdout);

            if (!stay_way_left2)
            {
                move_letf(&x2,size);
                bufer2[0] = 3;
                stay_way_right2 = true;
            } else {
                move_right(&x2,size);
                bufer2[0] = bufer2[1];
            }
            // reset předávané hodnoty
            data->tmp2 = 0;
        } else if (data->tmp2 == 2) { // pohyb hada nahoru
            fprintf(stdout, "UP\n");
            fflush(stdout);
            
            if (!stay_way_down2)
            {
                move_up(&y2,size);
                bufer2[0] = 2;
                stay_way_down2 = true;
            } else {
                bufer2[0] = bufer2[1];
                move_down(&y2,size);
            }
            // reset předávané hodnoty
            data->tmp2 = 0;
        } else if (data->tmp2 == 3) { // pohyb hada dolu
            fprintf(stdout, "DOWN\n");
            fflush(stdout);

            if (!stay_way_up2)
            {
                move_down(&y2,size);
                bufer2[0] = 1;
                stay_way_up2 = true;
            } else {
                move_up(&y2,size);
                bufer2[0] = bufer2[1];
            }
            // reset předávané hodnoty
            data->tmp2 = 0;            
        } else if (data->tmp2 == 4) { // pohyb hada doprava
            fprintf(stdout, "RIGHT\n");
            fflush(stdout);

            if (!stay_way_right2)
            {
                move_right(&x2,size);
                bufer2[0] = 4;
                stay_way_left2 = true;
            } else {
                move_letf(&x2,size);
                bufer2[0] = bufer2[1];
            }
            // reset předávané hodnoty
            data->tmp2 = 0;            
        }
        pthread_mutex_unlock(&mtx);

        // - kontrola jeslti jeden z hadů přejel pixel a připsání skóre do counteru
        // -> nastavení na false kontrolní proměné mňaky
        if (check_feed(x1,y1,pixx,pixy,size))
        {
            point_one++; // přičtení hráči body
            fprintf(stdout,"Počet bodů snake_one: %d\n",point_one);
            fflush(stdout);
            Check_food = false; // nastavení proměné na vytvoření nové mňamkky
            pthread_mutex_lock(&mtx);
            data->snake_feed = true;
            pthread_mutex_unlock(&mtx);
        } 
        if (check_feed(x2,y2,pixx,pixy,size))
        {
            point_two++; // přičtení hráči body
            fprintf(stdout,"Počet bodů snake_two: %d\n",point_two);
            fflush(stdout);
            Check_food = false; // nastavení proměné na vytvoření nové mňamkky
            pthread_mutex_lock(&mtx);
            data->snake_feed = true;
            pthread_mutex_unlock(&mtx);
        } 

        // vykreslení z buffru na obrazovku
        // // příprava a zápis dat na desku
        parlcd_write_cmd(parlcd_mem_base, 0x2c);
        for (int ptr = 0; ptr < 480*320 ; ptr++) {
            parlcd_write_data(parlcd_mem_base, fb[ptr]);
        }

        // předání lokálních dat do sdílených dat
        pthread_mutex_lock(&mtx);
        q = data->quit;
        data->player_one_points = point_one;
        data->player_two_points = point_two;
        pthread_mutex_unlock(&mtx);

        // kontrola, jeslti nějaký had nasbíral potřebný počet bodů
        if ( point_two >= MAXIMUM_POINTS_TO_WIN || point_one >= MAXIMUM_POINTS_TO_WIN)
        {
            pthread_mutex_lock(&mtx);
            q = data->quit = true;
            pthread_mutex_unlock(&mtx);
        }

        // zpomalení pohybů hadů
        usleep(300*1000);

    }   // end main while

    // fce na reset buffru na černo
    for(index = 0; index < 320*480; index++) {
        fb[index] = 0;
    }
    bool quit = false;

    // oznámení kdo vyhrál na standartní výstup
    pthread_mutex_lock(&mtx);
    if ((data->player_one_points == 16) || data->win_one_player)
    {
        printf("VYHRAL: hráč jedna\n");// vyhrál hráč jedna
        fprintf(stdout,"Please end the program with control key again 'q'\n");
        fflush(stdout);
        draw_end1(color_chars,fb);
        data->quit = false;
        
    } else if((data->player_two_points == 16) || data->win_two_player) {
        printf("VYHRAL: hráč dva\n");// vyhrál hráč dva
        fprintf(stdout,"Please end the program with control key again 'q'\n");
        fflush(stdout);
        draw_end2(color_chars,fb);
        data->quit = false;
        
    } else {
        quit = true;
    }
    pthread_mutex_unlock(&mtx);

    // vykreslení z buffru na obrazovku
    // // příprava a zápis dat na desku
    parlcd_write_cmd(parlcd_mem_base, 0x2c);
    for (int ptr = 0; ptr < 480*320 ; ptr++) {
        parlcd_write_data(parlcd_mem_base, fb[ptr]);
    }

    // čekání na ukončení programu uživatelem
    while(!quit)
    {
        pthread_mutex_unlock(&mtx);
        quit = data->quit;
        pthread_mutex_unlock(&mtx);
    }

    fprintf(stdout, "Control_Displej thread was closed!\n");
    fflush(stdout);

    ptr = 0;
    
    // reset obrazoky na černou obrazovku
    parlcd_write_cmd(parlcd_mem_base, 0x2c);
    for (int i = 0; i < 320 ; i++) {
        for (int j = 0; j < 480 ; j++) {
        fb[ptr]=0;
        parlcd_write_data(parlcd_mem_base, fb[ptr++]);
        }
    }

    return NULL;
}

void* Control_LED_DIOD(void* v)
{
    
    // zadefinování společních dat do vlákna
    SharedData* data = (SharedData*) v;
     
    pthread_mutex_lock(&mtx);
    bool q = false; // kontrolní proměná na ukončení průběhu vlánka
    pthread_mutex_unlock(&mtx);

    int counter= 0; // counter
    int mod = 0; // pomocná proměná
    
    unsigned char *mem_base; // proměná pro uložení namapované fyzické adresy LED DIODY
    
    // namapování adressy LED DIODY
    mem_base = map_phys_address(SPILED_REG_BASE_PHYS,SPILED_REG_SIZE,0);
    if (mem_base == NULL)
    {
        fprintf(stdout,"ERROR: WRONG mapping physical address\n");
        fflush(stdout);
    }
    
    // barva blikání led diody - zelená barva
    unsigned int color_LED_green = 0x00ff00; 

    // - červená barva 
    unsigned int color_LED_red = 0xff0000;

    //- modrá barva 
    unsigned int color_LED_blue = 0x0000ff;

    bool pauze = false; // kontrolní proměná pro pozdržení chodu vlákna

    // pauze dokud uživatel nezmáčkne příslušný symbol
    while(!pauze){
        
        pthread_mutex_lock(&mtx);
        pauze = data->pauze;
        pthread_mutex_unlock(&mtx);
    }

    while (!q)
    {
        counter++;
        mod = counter % 2; 
        
        // podmínky na střídání znaků
        if (mod == 1)
        {
            *(volatile uint32_t*)(mem_base + SPILED_REG_LED_RGB1_o) = color_LED_green;
        } else {
            *(volatile uint32_t*)(mem_base + SPILED_REG_LED_RGB1_o) = 0;
        }

        // rozvítí se led dioda, když sní mňamku - modře
        pthread_mutex_lock(&mtx);
        if (data->snake_feed)
        {
            *(volatile uint32_t*)(mem_base + SPILED_REG_LED_RGB1_o) = color_LED_blue;
            fprintf(stdout,"Had sežral mňamku\n");
            fflush(stdout);
            data->snake_feed = false;
        }
        pthread_mutex_unlock(&mtx);

        // zpomalení blikání led diody
        usleep(300*1000);
        
        // předání ukončovacích znaků
        pthread_mutex_lock(&mtx);
        q = data->quit;
        pthread_mutex_unlock(&mtx);

    } // end diody při led průběhu hada

    counter = 0; // reset counteru

    // led dioda bude několik vteřin blikat červeně, kdzž umře had
    // po 15 vteřinách by měla zhasnout
    pthread_mutex_lock(&mtx);
    bool dead = data->dead_snake;
    pthread_mutex_unlock(&mtx);

    while (counter <= 15 && dead)
    {
        counter++;
        mod = counter % 2; 
        if (mod == 1)
        {
            *(volatile uint32_t*)(mem_base + SPILED_REG_LED_RGB1_o) = color_LED_red;
        } else {
            *(volatile uint32_t*)(mem_base + SPILED_REG_LED_RGB1_o) = 0;
        }     
        // zpomalení blikání
        usleep(800*1000);  
        
        // reset dead_snake 
        pthread_mutex_lock(&mtx); 
        data->dead_snake = dead = false;
        pthread_mutex_unlock(&mtx);

        fprintf(stdout,"GAME OVER!\n");
        fflush(stdout);
    } // end LED - červená barva, když zemře had 

    // zhasnutí LED DIODY
    *(volatile uint32_t*)(mem_base + SPILED_REG_LED_RGB1_o) = 0;
    
    // zpráva o ukončení vlákna
    fprintf(stdout, "CONTROL_LED_DIOD thread was closed!\n");
    fflush(stdout);

    return NULL;
}

void* Control_LED_CHAIN(void* v)
{
    // sdílená data programu
    SharedData* data = (SharedData*) v;

    unsigned char *mem_base; // proměná pro uložení namapované fyzické adresy LED ŘÁDKU

    // namapování adressy LED ŘÁDKU
    mem_base = map_phys_address(SPILED_REG_BASE_PHYS,SPILED_REG_SIZE,0);
    if (mem_base == NULL)
    {
        fprintf(stderr,"ERROR: WRONG mapping physical address\n");
        fflush(stderr);
    }   

    int r = 0x01; // adresa do pravé části LED ŘÁDKU, udávající počet LED diod se má rozsvítit
    int g = 0x01; // adresa do levé části LED ŘÁDKU, udávající počet LED diod se má rozsvítit
    int counter1 = 1; // pomocné počítadlo pro výpočet správné adresy
    int counter2 = 1; // pomocné počítadlo pro výpočet správné adresy
    bool q = false; // kontrolní proměná pro ukončení vlákna
    int tmp1 = 0; // pomocná proměná pro zamezení opakování akce bez změny počtu bodů
    int tmp2 = 0; // pomocná proměná pro zamezení opakování akce bez změny počtu bodů

    // předání počátečních bodů obou hadů
    pthread_mutex_lock(&mtx);
    int player_points_one = data->player_one_points;
    int player_points_two = data->player_two_points;
    pthread_mutex_unlock(&mtx);

    while((player_points_one <= MAXIMUM_POINTS_TO_WIN) && (player_points_two <= MAXIMUM_POINTS_TO_WIN) && !q)
    {
        // výpočet kolik se má rozsvítit led diod pro hada 1
        if(player_points_one > 1 && tmp1 != player_points_one)
        {
            fprintf(stdout,"Hráč jedna skóroval\n");
            fflush(stdout);
            r += (counter1 << 1);
            counter1 *= 2;
            tmp1 = player_points_one;
        }
    
        // výpočet kolik se má rozsvítit led diod pro hada 2
        if(player_points_two > 1 && tmp2 != player_points_two)
        {
            fprintf(stdout,"Hráč dva skóroval\n");
            fflush(stdout);
            g += (counter2 << 1);
            counter2 *= 2;
            tmp2 = player_points_two;
        }
 
        // rozsvicení led diod prodle počtu bodů jednotlivých uživatelů
        if (player_points_two > 0 && player_points_one == 0)
        {
            *(volatile uint32_t*)(mem_base + SPILED_REG_LED_LINE_o) = g;
        } else if (player_points_one > 0 && player_points_two == 0) {
            *(volatile uint32_t*)(mem_base + SPILED_REG_LED_LINE_o) = (r<<16);
        } else if (player_points_two > 0 && player_points_one > 0) {
            *(volatile uint32_t*)(mem_base + SPILED_REG_LED_LINE_o) = ((r<<16) | g);
        }

        // kotrola jestli se má vypnout led dioda
        pthread_mutex_lock(&mtx);
        q = data->quit;
        pthread_mutex_unlock(&mtx);
        
        // předání získaných bodů obou hadů
        pthread_mutex_lock(&mtx);
        player_points_one = data->player_one_points;
        player_points_two = data->player_two_points;
        pthread_mutex_unlock(&mtx);

    }// end main while 

    // zhasnutí LED ŘÁDKU  
    *(volatile uint32_t*)(mem_base + SPILED_REG_LED_LINE_o) = 0;
    
    fprintf(stdout,"Control_LED_CHAIN thread was closed!\n");
    fflush(stdout);
    return NULL;
}

void* InputThread(void* v)
{
    // zadefinování společních dat do vlákna
    SharedData* data = (SharedData*) v;

    bool q = false; // kontrolní proměná na ukončení průběhu vlánka
    char c = 0; // proměná na uložení přijatého znaku od uživatele

    while (!q)
    {
        // načítání kontrolních znaků od uživatele
        c = getchar();

        // příkaz od uživatele na ukončení všech vláken
        if (c == 'q')
        {
            q = true;
            fprintf(stdout,"Close program was started\n");
            fflush(stdout);
        }  

        // kontrolní znak od uživatele pro spuštění hry
        if (c == 'p')
        {
            pthread_mutex_lock(&mtx);
            data->pauze = true;
            pthread_mutex_unlock(&mtx);
        }

        // příkazy od uživatele na změnu směru hada 2
        if (c == 'h')
        {
            pthread_mutex_lock(&mtx);
            data->tmp2 = 1;
            pthread_mutex_unlock(&mtx);
            fprintf(stdout,"Change direction snake 1 to left\n");
            fflush(stdout);
        } else if (c == 'u') {
            pthread_mutex_lock(&mtx);
            data->tmp2 = 2;
            pthread_mutex_unlock(&mtx);
            fprintf(stdout,"Change direction snake 1 to up\n");
            fflush(stdout);
        } else if (c == 'j') {
            pthread_mutex_lock(&mtx);
            data->tmp2 = 3;
            pthread_mutex_unlock(&mtx);
            fprintf(stdout,"Change direction snake 1 to down\n");
            fflush(stdout);
        } else if (c == 'k') {
            pthread_mutex_lock(&mtx);
            data->tmp2 = 4;
            pthread_mutex_unlock(&mtx);
            fprintf(stdout,"Change direction snake 1 to right\n");
            fflush(stdout);
        }

        // příkazy od uživatele na změnu směru hada 1
        if (c == 'a')
        {
            pthread_mutex_lock(&mtx);
            data->tmp = 1;
            pthread_mutex_unlock(&mtx);
            fprintf(stdout,"Change direction to left\n");
            fflush(stdout);
        } else if (c == 'w') {
            pthread_mutex_lock(&mtx);
            data->tmp = 2;
            pthread_mutex_unlock(&mtx);
            fprintf(stdout,"Change direction to up\n");
            fflush(stdout);
        } else if (c == 's') {
            pthread_mutex_lock(&mtx);
            data->tmp = 3;
            pthread_mutex_unlock(&mtx);
            fprintf(stdout,"Change direction to down\n");
            fflush(stdout);
        } else if (c == 'd') {
            pthread_mutex_lock(&mtx);
            data->tmp = 4;
            pthread_mutex_unlock(&mtx);
            fprintf(stdout,"Change direction to right\n");
            fflush(stdout);
        }
        
        // kontrolní znak od uživatele pro zjištění souřadnic hada
        if ( c == 'x')
        {
            fprintf(stdout,"Souřadnice hada:\n");
            fflush(stdout);
            pthread_mutex_lock(&mtx);
            data->coords_snake = true;
            pthread_mutex_unlock(&mtx);
        }
    
        // předání ukončovacího znaku
        pthread_mutex_lock(&mtx);
        data->quit = q;
        pthread_mutex_unlock(&mtx);
        
    } // end main while

    // zpráva o ukončení vlákna
    fprintf(stdout, "INPUT thread was closed!\n");
    fflush(stdout);

    return NULL;
}
