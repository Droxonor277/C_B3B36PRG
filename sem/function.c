#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <assert.h>

#include "messages.h"
#include "xwin_sdl.h"
#include "function.h"
#include <math.h>
#include "prg_serial_nonblock.h"



/*
Fce převzaté z podpůrných videích pana profesora Faigla na  
adrese: https://cw.fel.cvut.cz/wiki/courses/b3b36prg/resources/prgsem použité na chod programu,
přesněji řečeno použité na kominikaci mezi nukleo deskou a počítačem jsou tyto:

    void* my_alloc(size_t size); (celá)
    void computation_init(void); (celá a upravená k dalšímu použití)
    void computation_cleanup(void); (celá)
    bool is_computing(void); (celá)
    bool is_done(void); (celá)
    bool is_abort(void); (celá)
    void abort_comp(void); (celá)
    void enable_comp(void); (celá)
    bool set_compute(message *msg); (upravená)
    bool compute(message *msg); (celá)
    void update_data(const msg_compute_data *compute_data); (celá)
    void get_grid_size(w,h); (celá)
    void gui_init(void); (celá)
    void gui_cleanup(void); (upravená)
    void update_image(int w,int h, unsigned char* img); (celá)
    void gui_refresh(void); (celá)

- Děkuji za umožnění jejich použití;
*/


static struct {
    double c_re;// velikost reálné složky kostanty C
    double c_im;// velikost imaginární složky kostanty C
    int n;// počet iterací provedených na jednom pixelu při výpočtu

    double range_re_min; // minimální hodnota imaginární hodnoty
    double range_re_max; // maximální hodnota imaginární hodnoty
    double range_im_min; // maximální hodnota réálné hodnoty
    double range_im_max; // minimální hodnota réálné hodnoty

    int grid_w; // velikost šířky obrázku
    int grid_h; // velikost výšky obrázku

    int cur_x; // pomocný proměná reálné osy k orientaci kontrolní aklikace ve výpočtu
    int cur_y; // pomocný proměné imaginární osy k orientaci kontrolní aklikace ve výpočtu

    double d_re; // velikost šířky jednoho počítaného pixelu 
    double d_im; // velikost výšky jednoho počítaného poxelu

    int nbr_chunks; // počet chunků, které se budou počítat
    int cid; // počet spočítaných chunků
    double chunk_re; // počátečných reálná část souřadnice chunku, od které bude probíhat výpočet
    double chunk_im; // počátečných imaginární část souřadnice chunku, od které bude probíhat výpočet

    uint8_t chunk_n_re; // velikost šířky jednoho počítaného chunku
    uint8_t chunk_n_im; // velikost výšky jendoho počítaného chunku

    uint8_t* grid; // Odkaz na prostor, kde jsou uložena všechna spočítané iterace
    bool computing; // kontrolní proměná na kontrolu jestli nukleo počítá
    bool done; // kontrolní proměná na ohlášení konce výpočtu
    bool abort; // kontrolní proměná na vypnutí počítání nuklea
    bool set_compute; // kontrolní proměná na kontrolu jestli byly nukleu poslány data
    bool restart; // kontrolní proměná na zjištění jestli výpočet byl restartován 
    bool stop_animation; // kontrolní proměná na zastavení animace
    bool next; // kontrolní proměná, díky které mužeme pokračovat nebo začít od začátku animaci
    bool backup_calkulation; // kontrolní proměná na opačný výpočet 

    double ani_re; // posun s reálnou složkou konstanty C v animaci 
    double ani_im; // posun s imaginární složkou konstanty C v animaci 

    double shift_re; // proměná na zvýšení posunu reálné části konstanty c při animaci
    double shift_im; // proměná na zvýšení posunu imaginární části konstanty c při animaci

    double save_c_re; // uložení reálné složky v poslední posun animace
    double save_c_im; // uložení imaginární složky v poslední posun animace


} comp = {
    .c_re = -0.4, 
    .c_im = 0.6,  

    .n = 60, 

    .range_im_min = -1.1,
    .range_im_max = 1.1, 
    .range_re_max = 1.6, 
    .range_re_min = -1.6, 

    .grid = NULL,
    .grid_h = 480, 
    .grid_w = 640,  

    .cur_x = 0,
    .cur_y = 0,

    .nbr_chunks = 0,

    .chunk_n_re = 64, 
    .chunk_n_im = 48, 



    .computing = false, 
    .done = false, 
    .abort = false, 
    .set_compute = false, 
    .restart = false, 
    .stop_animation = true, 
    .next = false,
    .backup_calkulation = false,

    .ani_re = 0.005, 
    .ani_im = 0.005, 

    .shift_re = 0.001,
    .shift_im = 0.001,

    .save_c_re = 0,
    .save_c_im = 0
};

static struct 
{
    int w;
    int h;
    unsigned char *img;

} gui = { .img = NULL };

void* my_alloc(size_t size)
{
    void *ret = malloc(size);
    if (!ret)
    {
        fprintf(stderr, "ERROR: cannot malloc!\n");
        exit(101);
    }
    return ret;

}

void computation_init(void)
{
    comp.grid = my_alloc(comp.grid_w * comp.grid_h);
    comp.d_re = (comp.range_re_max - comp.range_re_min) / (1. * comp.grid_w);
    comp.d_im = -(comp.range_im_max - comp.range_im_min) / (1. * comp.grid_h);
    comp.nbr_chunks = (comp.grid_w * comp.grid_h) / (comp.chunk_n_re * comp.chunk_n_im);

}

void computation_cleanup(void)
{
    if (comp.grid)
    {
        free(comp.grid);
    }
    comp.grid = NULL;
    comp.cid = 0;
    comp.cur_x = 0;
    comp.cur_y = 0;
    comp.d_re = 0;
    comp.d_im = 0;
    comp.nbr_chunks = 0;
}

bool is_computing(void)
{
    return comp.computing;
}

bool is_done(void)
{
    return comp.done;
}

bool is_aborted(void)
{
    return comp.abort;
}

void abort_comp(void)
{
    comp.abort = true;
}

void print_info_chunk(void)
{
    fprintf(stdout, "INFO: Chunk %d was computed!\r\n",comp.cid);
    fflush(stdout);
}

void enable_comp(void)
{
    comp.abort = false;
}

void restart_compute(void)
{
    if (comp.computing)
    {
        comp.cid = 0;
        comp.cur_x = comp.cur_y = 0; // start computation of the whole image
        comp.chunk_re = comp.range_re_min; // upper-"left" corner
        comp.chunk_im = comp.range_im_max; // "upper"-left corner
        comp.restart = true;
        comp.computing = false;
        fprintf(stdout, "INFO: Compute was restarted\r\n");
        fflush(stdout);
    } else {
        fprintf(stderr, "WARN: Don't need restart compute\r\n");
        fflush(stderr);
    }
}

bool is_set_data(void)
{
    return comp.set_compute;
}

bool set_compute(message *msg)
{
    // kotrola jeslti byl spuštěn výpočet 
    bool ret = !is_computing();
    
    // Nastavení dat k výpočtu 
    if (ret)
    {
        msg->type = MSG_SET_COMPUTE;
        msg->data.set_compute.c_re = comp.c_re; 
        msg->data.set_compute.c_im = comp.c_im;
        msg->data.set_compute.d_re = comp.d_re;
        msg->data.set_compute.d_im = comp.d_im;
        msg->data.set_compute.n = comp.n;
        comp.done = false;
        comp.set_compute = true;
    } else if (comp.restart){
        msg->type = MSG_SET_COMPUTE;
        msg->data.set_compute.c_re = comp.c_re; 
        msg->data.set_compute.c_im = comp.c_im;
        msg->data.set_compute.d_re = comp.d_re;
        msg->data.set_compute.d_im = comp.d_im;
        msg->data.set_compute.n = comp.n;
        comp.done = false;
        comp.set_compute = true;
        comp.restart = false; // resetování proměné
        ret = true;        
    }
    return ret;
}

bool compute(message *msg)
{
    
    if (!is_computing()) // first chunk 
    {
        comp.cid = 0;
        comp.computing = true;
        comp.cur_x = comp.cur_y = 0; // start computation of the whole image
        comp.chunk_re = comp.range_re_min; // upper-"left" corner
        comp.chunk_im = comp.range_im_max; // "upper"-left corner
        msg->type = MSG_COMPUTE;
    } else {// next chunk
        comp.cid += 1; // chunk_id +1

        // kontrola jestli se nepřesáhl počet chunků obrázku
        if (comp.cid < comp.nbr_chunks)
        {
            comp.cur_x += comp.chunk_n_re; // posunutí kursoru od +64 (velisko jednoho chunku)
            comp.chunk_re += comp.chunk_n_re * comp.d_re; // posunutí počátečních souřadnic o velikost jednoho chunku * posun
            
            // pokudli se přehoupne počet spočítaných pixelů v řádku 
            if (comp.cur_x >= comp.grid_w)
            { 
                comp.chunk_re = comp.range_re_min; // nastavý se počáteční souřadnice výpočtu na minimální souřadnice
                comp.chunk_im += comp.chunk_n_im * comp.d_im; // odečte se od počátečných osuřadnice d_im * chunk_n_im, čím se posuneme o chunk_n_im řádků dolů
                comp.cur_x = 0; // reset kursoru
                comp.cur_y += comp.chunk_n_im; // přičtení chunk_n_im k cur_y pro zapamatování, kde jsme
            } 
            msg->type = MSG_COMPUTE;
        } else { // all has been computed
            fprintf(stderr, "WARN: All has been computed but nukleo still compute\r\n");
            fflush(stderr);
        }
    } 

    // kotrola jestli se mají načíst další data na další výpočet
    if ( comp.computing && msg->type == MSG_COMPUTE)
    {
        msg->data.compute.cid = comp.cid; // chunk_id
        msg->data.compute.re = comp.chunk_re; // počáteční souřadnice reálné složky
        msg->data.compute.im = comp.chunk_im; // počáteční souřadnice imaginární složky
        msg->data.compute.n_re = comp.chunk_n_re; // předání počtu pixelů reálné složky chunku
        msg->data.compute.n_im = comp.chunk_n_im; // předání počtu pixelů imaginární složky chunku
    }
    fprintf(stdout,"%f - pocáteční re, %f - pocáteční im", comp.chunk_re,comp.chunk_im);
    fflush(stdout);
    return is_computing();

}

void update_data(const msg_compute_data *compute_data)
{
    if (compute_data->cid == comp.cid)
    {
        // Načítání iterací do matice a výpočtem na jaký idx se to má uložit 
        const int idx = comp.cur_x + compute_data->i_re + (comp.cur_y + compute_data->i_im) * comp.grid_w;
        if (idx >= 0 && idx < (comp.grid_w * comp.grid_h))
        {
            comp.grid[idx] = compute_data->iter;
        }

        // kontrola jestli za další načtení skončí výpočet
        if ((comp.cid +1) >= comp.nbr_chunks && (compute_data->i_re +1) == comp.chunk_n_re && (compute_data->i_im + 1 ) == comp.chunk_n_im )
        {
            comp.done = true;
            comp.computing = false;
        }
        xwin_poll_events();
    } else {
        fprintf(stderr,"ERROR: Received chunk with unenxpected chunk id (cid)\r\n");
        fflush(stderr);
    }
}

void get_grid_size(int *w,int *h)
{
    *w = comp.grid_w;
    *h = comp.grid_h;
}

void gui_init(void)
{
    get_grid_size(&gui.w, &gui.h);
    gui.img = my_alloc (gui.w * gui.h* 3);
    xwin_init(gui.w,gui.h);
}

void gui_black_screen(void)
{
    for (int i = 0; i < comp.grid_w * comp.grid_h; i++)
    {
        comp.grid[i] = 0;
    }    

}

void gui_cleanup(void)
{
    for (int i = 0; i < gui.w * gui.h; i++)
    {
        gui.img[i] = 0;
        gui.img[i+1] = 0;
        gui.img[i+2] = 0;
        xwin_poll_events();
    }    
    xwin_redraw(gui.w,gui.h,gui.img); // přepsání okna na černo



    if (gui.img)
    {
        free(gui.img);
        gui.img = NULL;
    }
    xwin_close();
}

void update_image(int w,int h, unsigned char* img)
{
    for (int i = 0; i < w * h; ++i )
    {
        const double t = 1.0 * comp.grid[i] / (comp.n + 1.0); // je to tam, abych nedělil nulou 
        *(img++) = (9 * (1 - t) * pow(t, 3)) * 255;
        *(img++) = (15 * pow((1 - t), 2) * pow(t, 2)) * 255;
        *(img++) = (8.5 * pow((1 - t), 3) * t) * 255;
    }
}

void gui_refresh(void)
{
    if(gui.img)
    {
        update_image(gui.w,gui.h,gui.img);
        xwin_redraw(gui.w, gui.h,gui.img);
        xwin_poll_events();
    }
}

void load_cre(void)
{
    scanf("%lf",&(comp.c_re));

}

void load_cim(void)
{
    scanf("%lf", &(comp.c_im));


}

void load_max_re(void)
{
    scanf("%lf",&(comp.range_re_max));

}

void loadminre(void)
{
    scanf("%lf",&(comp.range_re_min));
}

void print_re_interval_values(void)
{
    fprintf(stdout,"%f - minimum values real values, %f - maximum values real values"
                    ", %f - the width of one pixel\r\n", comp.range_re_min, comp.range_re_max, comp.d_re);
    fflush(stdout);    
}

void comp_d_re(void)
{
    comp.d_re = (comp.range_re_max - comp.range_re_min) / (1. * comp.grid_w);
}

void print_C(void)
{
    fprintf(stdout,"%f - cre, %f - cim\r\n", comp.c_re, comp.c_im);
    fflush(stdout);
}

void loadmin_im(void)
{
    scanf("%lf",&(comp.range_im_min));
}

void loadmax_im(void)
{
    scanf("%lf",&(comp.range_im_max));
}

void print_im_interval_values(void)
{
    fprintf(stdout,"%f - minimum values imaginary values, %f - maximum values imaginary values"
                    ", %f - the height  of one pixel\r\n", comp.range_im_min, comp.range_im_max, comp.d_im);
    fflush(stdout);   
}

void comp_d_im(void)
{
    comp.d_im = -(comp.range_im_max - comp.range_im_min) / (1. * comp.grid_h);
}

void reset_chunk(void)
{
    
    if (!(comp.computing))
    {
        comp.cid = 0;
        fprintf(stderr, "INFO: Get reset chunk_id %d\r\n", comp.cid);
    } else {
        fprintf(stderr, "WARN: Cannot reset chunk_id, when nulkeo still calculate\r\n");
    }
}

void load_number_iteration(void)
{
    scanf("%d",&(comp.n));
}

void print_number_iteration(void)
{
    fprintf(stdout,"%d - the number of iteration\r\n", comp.n);
    fflush(stdout); 
}

void load_new_re_shift(void)
{
    scanf("%lf",&(comp.shift_re));
}

void load_new_im_shift(void)
{
    scanf("%lf",&(comp.shift_im));
}

void print_animation_change_shift(void)
{
    fprintf(stdout,"%lf - current animation real shift value , %lf - current animation imagination shift value\n",
                     comp.shift_re, comp.shift_im);
    fflush(stdout);      
}

void plus_shift_re(void)
{
    comp.ani_re += comp.shift_re;
}

void minus_shift_re(void)
{
    comp.ani_re -= comp.shift_re;
}

void plus_shift_im(void)
{
    comp.ani_im += comp.shift_im;
}

void minus_shift_im(void)
{
    comp.ani_im -= comp.shift_im;
}

void print_animation_current_shirt(void)
{
    fprintf(stdout,"%lf - current animation real shift value , %lf - current animation imagination shift value\n",
                     comp.ani_re, comp.ani_im);
    fflush(stdout);      
}

void load_whole_new_shift_re(void)
{
    scanf("%lf", &(comp.ani_re));
}

void load_whole_new_shift_im(void)
{
    scanf("%lf", &(comp.ani_im));
}

void print_entered_data(void)
{
    fprintf(stdout, "=========================================================================================\n"
                    "|                                    Entered data are:                                  |\n"
                    "|=======================================================================================|\n"
                    "| The real part of the constant C:                                          | %lf |\n"                                     
                    "|===========================================================================|===========|\n"
                    "| The imaginary part of the constant C:                                     | %9lf |\n"
                    "|===========================================================================|===========|\n"
                    "| The number of iteration:                                                  | %9d |\n"
                    "|===========================================================================|===========|\n"
                    "| The maximal compute value on real axis:                                   | %9lf |\n"
                    "|===========================================================================|===========|\n"
                    "| The minimal compute value on real axis:                                   | %9lf |\n"
                    "|===========================================================================|===========|\n"
                    "| The maximal compute value on imaginary axis:                              | %9lf |\n"
                    "|===========================================================================|===========|\n"
                    "| The minimal compute value on imaginary axis:                              | %9lf |\n"
                    "|===========================================================================|===========|\n"
                    "| The size of the graphics window width:                                    | %9d |\n"
                    "|===========================================================================|===========|\n"
                    "| The size of the graphics window height:                                   | %9d |\n"
                    "|===========================================================================|===========|\n"
                    "| The size of the height of one counted chunk:                              | %9d |\n"
                    "|===========================================================================|===========|\n"
                    "| The size of the width of one counted chunk:                               | %9d |\n"
                    "|===========================================================================|===========|\n"
                    "| The set value for the shift of the real component constant C:             | %9lf |\n"
                    "|===========================================================================|===========|\n"
                    "| The set value for the shift of the imaginary component constant C:        | %9lf |\n"
                    "|===========================================================================|===========|\n"
                    "| The current value which user can increace or decrease real shift:         | %9lf |\n"
                    "|===========================================================================|===========|\n"
                    "| The current value which user can increace or decrease imaginary shift:    | %9lf |\n"
                    "|===========================================================================|===========|\n"
                    "| The current width of the one pixel:                                       | %9lf |\n"
                    "|===========================================================================|===========|\n"
                    "| The current height of the one pixel:                                      | %9lf |\n"
                    "|===========================================================================|===========|\n"
                    "| The current chunk_id:                                                     | %9d |\n"
                    "|===========================================================================|===========|\n"
                    "| The current start real part coord chunk:                                  | %9lf |\n"
                    "|===========================================================================|===========|\n"
                    "| The current start imaginary part coord chunk:                             | %9lf |\n"
                    "|===========================================================================|===========|\n"
                    "| The current save data from the real part of C constant:                   | %9lf |\n"
                    "|===========================================================================|===========|\n"
                    "| The current save data from the imaginary part of C constant:              | %9lf |\n"
                    "=========================================================================================\r\n",
                    comp.c_re,comp.c_im,comp.n,comp.range_re_max, comp.range_re_min, comp.range_im_max,
                    comp.range_im_min, comp.grid_w,comp.grid_h,comp.chunk_n_re, comp.chunk_n_im,
                    comp.ani_re, comp.ani_im, comp.shift_re,comp.shift_im,comp.d_re, comp.d_im,
                    comp.cid,comp.chunk_re, comp.chunk_im, comp.save_c_re, comp.save_c_im);
}

void print_control_key(void)
{
    fprintf(stdout, "==========================================================================================\n"
                    "| Control key |                The meaning of the given control keys                     |\n"    
                    "|=============|==========================================================================|\n"    
                    "|    'g'      | to get version nukleo program                                            |\n"
                    "|=============|==========================================================================|\n"
                    "|    '1'      | to set starting data and start compute the chunk of picture on nukleu    |\n"    
                    "|=============|==========================================================================|\n"    
                    "|    'a'      | to abort the calculation of picture's chunk on nukleo                    |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'r'      | to reset calculation on start pozition                                   |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'u'      | to start settings menu parameters                                        |\n"
                    "|=============|==========================================================================|\n" 
                    "|    't'      | to save picture as a .ppm file into the folder where is boss program     |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'f'      | to start animation image on pc                                           |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'h'      | to stop the animation                                                    |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'i'      | to inform the user about control key                                     |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'l'      | to delete actualy buffer with the calculated data                        |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'p'      | to refresh grafic window                                                 |\n"
                    "|=============|==========================================================================|\n" 
                    "|    '+'      | to increase real shift by set value                                      |\n"
                    "|=============|==========================================================================|\n" 
                    "|    '-'      | to decrease real shift by set value                                      |\n"
                    "|=============|==========================================================================|\n" 
                    "|    '*'      | to decrease imaginary shift by set value                                 |\n"
                    "|=============|==========================================================================|\n" 
                    "|    '/'      | to increase imaginary shift by set value                                 |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'o'      | to print everything data which was set                                   |\n"
                    "|=============|==========================================================================|\n" 
                    "|    's'      | to prepared set data for computing to nukleo                             |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'c'      | to computed and display image to grafic window on pc                     |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'm'      | to set control variable and we can continue the animation                |\n" 
                    "|=============|==========================================================================|\n" 
                    "|    'n'      | to change animation on oposite way                                       |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'q'      | to quit both programs                                                    |\n"
                    "==========================================================================================\r\n"); 
    fflush(stdout);
}

void print_start(void)
{
    fprintf(stdout, "Boss aplication was started!\r\n");
    fflush(stdout);
    fprintf(stdout, "==========================================================================================\n"
                    "| Control key |                The meaning of the given control keys                     |\n"    
                    "|=============|==========================================================================|\n"    
                    "|    'g'      | to get version nukleo program                                            |\n"
                    "|=============|==========================================================================|\n"
                    "|    '1'      | to set starting data and start compute the chunk of picture on nukleu    |\n"    
                    "|=============|==========================================================================|\n"    
                    "|    'a'      | to abort the calculation of picture's chunk on nukleo                    |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'r'      | to reset calculation on start pozition                                   |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'u'      | to start settings menu parameters                                        |\n"
                    "|=============|==========================================================================|\n" 
                    "|    't'      | to save picture as a .ppm file into the folder where is boss program     |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'f'      | to start animation image on pc                                           |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'h'      | to stop the animation                                                    |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'i'      | to inform the user about control key                                     |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'l'      | to delete actualy buffer with the calculated data                        |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'p'      | to refresh grafic window                                                 |\n"
                    "|=============|==========================================================================|\n" 
                    "|    '+'      | to increase real shift by set value                                      |\n"
                    "|=============|==========================================================================|\n" 
                    "|    '-'      | to decrease real shift by set value                                      |\n"
                    "|=============|==========================================================================|\n" 
                    "|    '*'      | to decrease imaginary shift by set value                                 |\n"
                    "|=============|==========================================================================|\n" 
                    "|    '/'      | to increase imaginary shift by set value                                 |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'o'      | to print everything data which was set                                   |\n"
                    "|=============|==========================================================================|\n" 
                    "|    's'      | to prepared set data for computing to nukleo                             |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'c'      | to computed and display image to grafic window on pc                     |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'm'      | to set control variable and we can continue the animation                |\n" 
                    "|=============|==========================================================================|\n" 
                    "|    'n'      | to change animation on oposite way                                       |\n"
                    "|=============|==========================================================================|\n" 
                    "|    'q'      | to quit both programs                                                    |\n"
                    "==========================================================================================\r\n"); 
    fflush(stdout);          
}

void compute_Julian_set(void)
{

    int imageindex = 0; // index v obrazku, díky němuž se pohybujeme v poli o danout hodnotu

    float tmp_re = 0; // nastavení lokální reálné hodnoty komplexního čísla na pixelu 
    float tmp_im = 0; // nastavení lokální imaginární hodnoty komplexního čísla na pixelu 

    int iteration = 0; // pocet porbehlich iteraci

    // vypsání kontrolních rozměrů
    fprintf(stdout, "INFO: The image has pixels %d\n", comp.grid_h* comp.grid_w);
    fprintf(stdout, "INFO: he area to be calculated is from %f to %f on the real axis\n", comp.range_re_min,comp.range_re_max);
    fprintf(stdout, "INFO: he area to be calculated is from %f to %f on the imaginary axis\n", comp.range_im_min,comp.range_im_max);
    fprintf(stdout, "INFO: Constatnt C has real part: %f and imaginary part: %f\n", comp.c_re,comp.c_im);
    
    double r = 0; // červená složka
    double g = 0; // zelená složka
    double b = 0; // modrá složka    
    
    tmp_im = comp.range_im_max; // nastavení 1. pixelu, kde se nastavuje jeho imaginární složka do leveho horniho pixelu

    for (int m = 0; m < comp.grid_h; m++)
    {
        tmp_re = comp.range_re_min; // 1. pixel v řádku se resetuje na minimální reálnou hodnotu do leveho horniho pixelu
        
        if (m >= 1)
        {
            tmp_im += comp.d_im; // bude snižovat výšku až od druhé řady pixelů
        }
        
        for (int j = 0; j < comp.grid_w; j++)
        {
            if (j >= 1)
                tmp_re += comp.d_re; // posun v řádku po pixelech
         

            // kontrola jestli cislo patri do Julian set 
            //iteration 
            comp.grid[iteration] = computJulianSet(tmp_re, tmp_im, comp.n, comp.c_re, comp.c_im); // počet vrácených, které prošli iteracemi
            if (comp.grid[iteration] != comp.n) // nepatri do Julian set
            {

                distribution_of_color(comp.n, comp.grid[iteration], &r, &g, &b);
                gui.img[imageindex] =  r; //červená složka 
                gui.img[imageindex + 1] = g; // zelená složka
                gui.img[imageindex + 2] = b; // modrá složka 
            }
            else { // jinak pixel patří do Julian set and pixel have black as color

                gui.img[imageindex] = 0; //červená složka 
                gui.img[imageindex + 1] = 0; // zelená složka
                gui.img[imageindex + 2] = 0; // modrá složka 
            }
            imageindex += 3; // posun o 3 v poli
            
            iteration++;

        }
        
        // prepsani do obrazku vypocitane hodnoty
        xwin_redraw(comp.grid_w,comp.grid_h,gui.img);
        xwin_poll_events(); 
    }
}

void distribution_of_color(int k, int i, double* r, double* g, double* b)
{
    double t = (double)i / k;
    *r = (9 * (1 - t) * pow(t, 3)) * 255;
    *g = (15 * pow((1 - t), 2) * pow(t, 2)) * 255;
    *b = (8.5 * pow((1 - t), 3) * t) * 255;
}

int computJulianSet(double re, double im, int k, double cre, double cim)
{

    int iteration = 0; // počet iterací, které vrátí
    double tmpRe = 0; // pomocná proměná
    double z_abs = 0; // absolutní hodnota
    while (iteration < k)
    {
        iteration++; // zvýšení počtu iterací v jednom chodu

        // umocnění komplexního čísla 
        tmpRe = re;
        re = re * re + (-1)*(im)*(im);
        im = 2 * tmpRe * im;

        // sečtení komplexního čísla
        re = re + cre;
        im = im + cim;
   
        // výpočet absolutní hodnoty z komlexního čísla
        z_abs = sqrt((re) * (re)+(im) * (im)); 

        // podmínka na ukončení iterací a vrácení hodnoty z níž se bude počítat barva
        if (z_abs >= 2)
        {
            break;
        }
        
    }

    return iteration;
}

void save_image(void)
{
    FILE* OutputImage = fopen("Julian_set.ppm", "wb"); // otevření souboru pro binární zápis
    fprintf(OutputImage, "P6\n"); // zapsání nutného názvu do souboru obrázku a odřádkování 
    fprintf(OutputImage, "%d %d\n", comp.grid_w, comp.grid_h); // zapsání nutného záznamu šířky a výšky a odřádkování
    fprintf(OutputImage, "%d\n", 255); // zapsání nutného záznamu Maximální hodnota intenzity dané složky pixelu

    fwrite(gui.img,1,comp.grid_w * comp.grid_h * 3, OutputImage); // výpis do souboru
    fclose(OutputImage); // uzavření souboru po zapsání 
}

void stop_animation(void)
{
    comp.stop_animation = false;
}

bool is_animated(void)
{
    return comp.stop_animation;
}

void continue_animation(void)
{
    comp.next = !(comp.next);
}

void oposite_animation(void)
{
    comp.backup_calkulation = !(comp.backup_calkulation);
}

void animation_image(void)
{
    
    int imageindex = 0; // index v obrazku, díky němuž se pohybujeme v poli o danout hodnotu

    float tmp_re = 0; // nastavení lokální reálné hodnoty komplexního čísla na pixelu 
    float tmp_im = 0; // nastavení lokální imaginární hodnoty komplexního čísla na pixelu 

    int iteration = 0; // pocet porbehlich iteraci

    float tmp_c_re = 0;
    float tmp_c_im = 0;
    if (!comp.next)
    {
        tmp_c_re = comp.c_re;
        tmp_c_im = comp.c_im;
    } else {
        tmp_c_re = comp.save_c_re;
        tmp_c_im = comp.save_c_im;
    }

    comp.stop_animation = true;

    // vypsání kontrolních rozměrů
    fprintf(stdout, "INFO: The image has pixels %d\n", comp.grid_h* comp.grid_w);
    fprintf(stdout, "INFO: he area to be calculated is from %f to %f on the real axis\n", comp.range_re_min,comp.range_re_max);
    fprintf(stdout, "INFO: he area to be calculated is from %f to %f on the imaginary axis\n", comp.range_im_min,comp.range_im_max);
    fprintf(stdout, "INFO: Constatnt C has real part: %f and imaginary part: %f\n", comp.c_re,comp.c_im);
    fprintf(stdout,"INFO: Animation was started!\r\n");
    fprintf(stdout, "INFO: For correct end program please press 'h' control key\n");

    fflush(stdout);
    
    double r = 0; // červená složka
    double g = 0; // zelená složka
    double b = 0; // modrá složka    
    
    while (is_animated())
    {

        tmp_im = comp.range_im_max; // nastavení 1. pixelu, kde se nastavuje jeho imaginární složka do leveho horniho pixelu

        for (int m = 0; m < comp.grid_h; m++)
        {
            tmp_re = comp.range_re_min; // 1. pixel v řádku se resetuje na minimální reálnou hodnotu do leveho horniho pixelu
            
            if (m >= 1)
            {
                tmp_im += comp.d_im; // bude snižovat výšku až od druhé řady pixelů
            }

            for (int j = 0; j < comp.grid_w; j++)
            {
                if (j >= 1)
                    tmp_re += comp.d_re; // posun v řádku po pixelech
            
                // kontrola jestli cislo patri do Julian set 
                comp.grid[iteration] = computJulianSet(tmp_re, tmp_im, comp.n, tmp_c_re,tmp_c_im); // počet vrácených, které prošli iteracemi
                if (comp.grid[iteration] != comp.n) // nepatri do Julian set
                {

                    distribution_of_color(comp.n, comp.grid[iteration] , &r, &g, &b);
                    gui.img[imageindex] =  r; //červená složka 
                    gui.img[imageindex + 1] = g; // zelená složka
                    gui.img[imageindex + 2] = b; // modrá složka 
                }
                else { // jinak pixel patří do Julian set and pixel have black as color

                    gui.img[imageindex] = 0; //červená složka 
                    gui.img[imageindex + 1] = 0; // zelená složka
                    gui.img[imageindex + 2] = 0; // modrá složka 
                }
                imageindex += 3; // posun o 3 v poli
                iteration++;
            }
         

        }
    
        // prepsani do obrazku vypocitane hodnoty
        xwin_redraw(comp.grid_w,comp.grid_h,gui.img);
        xwin_poll_events();    

        if (!comp.backup_calkulation)
        {
            tmp_c_re += comp.ani_re;
            tmp_c_im += comp.ani_im;
        } else {
            tmp_c_re -= comp.ani_re;
            tmp_c_im -= comp.ani_im;            
        }
        imageindex = 0;
        iteration = 0;
    }

    comp.save_c_re = tmp_c_re;
    comp.save_c_im = tmp_c_im;
}
