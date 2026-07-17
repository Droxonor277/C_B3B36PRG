#include <stdio.h>
#include <stdbool.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include "prg_serial_nonblock.h"

#define myDEVICE "/dev/ttyS3"

// Fce vrací na výstup informace o používání nuklea 
void* outputThread(void*);

// Fce počítá(odhaduje) periodu blikání
void* computePeriodThread(void*);

// Fce zprovozňuje vstup z klávesnice a podmět na ukončování vláken
void* inputThread(void*);

// Fce čte znaky z nulkea a ukládáje do struktury programu 
void* readThread(void*);

// Fce vrací po 5s odhad periody a resetuje ji
void* sleepThread(void*);

// Fce dává terminál do stavu raw 
void call_termios(int reset);

// Struktura na organizaci programu
typedef struct
{
    int preriod; // odhadnutá perioda blikání
    int counter; // counter na počet 'x' při blikání
    char accepted_charakter; // přijmutý znak z nuklea
    char send_charakter; // poslaný znak pc
    bool quit; // proměná na ukončení pogramu
    bool is_serial_open; // proměná na ukončení seriové linky
    int nukleo; // adressa pro otevření nuklea
    bool check; // kontrolní proměná na odhad periody
    char* state[3]; // Kontrolní proměná pro ohlášení stavu LED diody
}SharedData;

// Zadefinování všech proměných do na jejich pořáteční pozici 
SharedData initData()
{
    SharedData ret;
    ret.quit = false;
    ret.send_charakter = '?';
    ret.is_serial_open = false;
    ret.nukleo = 0;    
    ret.accepted_charakter = ' ';
    ret.preriod = 0;
    ret.counter = 0;
    ret.check = false;
    ret.state[3] = "";
    return ret;
}

// Zadefinování potřebných fcí
pthread_mutex_t mtx;
pthread_cond_t condvar;

int main()
{
    call_termios(0); // spuštění serial raw portu
   
    SharedData data = initData(); // Dání dat do programu
     
    // zadefinování fcí na ochranu dat ve vláknu a fce na vysílání signálu mezi vlákny
    pthread_mutex_init(&mtx, NULL);
    pthread_cond_init(&condvar, NULL);

    // zadefinování počtu a obecně zadefinování vláken použitých v budoucnosti
    enum {INPUT,OUTPUT, SLEEP, NUM_THREADS };
    void* (*thr_threads[])(void*) = {inputThread, outputThread, sleepThread };

    // Vytvoření Vláken
    pthread_t threads[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_create(&threads[i], NULL, thr_threads[i], &data);

    // Počáteční výtisk čekající na další instrukce 
    printf("\rLED off,"
        "send: '?',"
        "received: '',"
        "T = 0 ms,"
        "ticker = 0");

    // Koretní ukončení všech vláken  
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }    
    
    // Konečný výtisk čekající na další instrukce 
    printf("\rLED %3s,"
        "send: '%c',"
        "received: '%c',"
        "T = %4d ms,"
        "ticker = %4d",
        data.state[3],
        data.send_charakter,
        data.accepted_charakter,
        data.counter,
        data.preriod);
    fflush(stdout);
    
    call_termios(1); // Zavření serial raw portu
    return 0;
}

void* sleepThread(void *v)
{
    
    // Zavední globální struktury do vlákna
    SharedData* data = (SharedData*) v; 
    pthread_mutex_lock(&mtx);

    // Fce čekající na signál z fce output na provedení výpočtu periody 
    pthread_cond_wait(&condvar, &mtx);
    bool q = data->quit; // Kontrolní perioda
    pthread_mutex_unlock(&mtx);
    
    // Opakované zjištění chodu výpočtu periody
    while(!q)
    {
        // Uspání programu na 5s pro možnost poté načíst periodu blikání 
        sleep(5);  
        pthread_mutex_lock(&mtx);
        //pthread_cond_wait(&condvar, &mtx);
        
        // Počítadlo odhadnuté periody blikání
        if (data->counter != 0)
        {
            data->preriod = (5000/(data->counter*2)); 
        } else {
            data->preriod = 0;
        }
        data->counter = 0; // Resetování countru 'x'
        data->check = false; // Kontrolní proměná na možnost znovu počítat periodu
        q = data->quit; // Ukončovací proměná pro vlákno
        pthread_mutex_unlock(&mtx);
    }
    
    return 0;
}

void* outputThread(void *v)
{
    
    // Zavedení struktury do vlákna
    SharedData* data = (SharedData*) v;
    bool q = false; // kontrolní proměná na ukončení chodu vlákna

    // Nastavení hodnot ve fci a zároveň otervení seriového portu
    const char* device = myDEVICE; // adresa portu
    pthread_mutex_lock(&mtx);
    data->nukleo = serial_open(device); // otevření seriové linky
    pthread_mutex_unlock(&mtx); 
  
    // Kontrola na správně otevření seriové linky 
    if (data->nukleo == -1) 
    {
        fprintf(stderr, "\rERROR: Cannot open device %s\n", device);
        pthread_mutex_lock(&mtx);
        data->quit = true; 
        pthread_mutex_unlock(&mtx);
    }  

    // Ochrana vstupu z klavesnice před tím než se načte zařízení v této fci  
    pthread_mutex_lock(&mtx);
    data->is_serial_open = true;
    pthread_mutex_unlock(&mtx);
    
    // Proměná na uvedení velikosti do fce a možnost z té fce do ní uložit načtený znak
    char cc = 0; 
    
    // Chod vlákna
    while(!q)
    {
        pthread_mutex_lock(&mtx);
        
        // Načítání znaků se sériové linky
        int rr = serial_getc_timeout(data->nukleo, 0.3, &cc);   
            
            // Kontrola na overeni korektnosti vstupu 
            if (rr == -1)
            {
                fprintf(stderr, "ERROR, Comunication");
                q = true;
                data->quit = q;     
            }

        // Uložení přijatého znaku ze sériové linky     
        data->accepted_charakter = cc;
        cc = 0;

        // Počítadlo přijatých 'x'
        if (data->accepted_charakter == 'x')
        {
            if (data->check == false)
            {
                pthread_cond_signal(&condvar);
                data->check = true;
            }
            data->counter++;
        }
        
        // Kontrola stavu LED diody
        if ((data->accepted_charakter == 'a' && data->send_charakter == 's') ||
             data->accepted_charakter == 'x')
        {
            data->state[3] = "on";
        }
        if ((data->accepted_charakter == 'a' && data->send_charakter == 'e') || 
            data->accepted_charakter == 'o' || data->accepted_charakter == 'b')
        {
            data->state[3] = "off";
        }   
        
        // Výstup na terminál s informacemi o chodu Nuklea
        if (!q && cc != data->accepted_charakter)
        {
            printf("\rLED %3s,"
                "send: '%c',"
                "received: '%c',"
                "T = %4d ms,"
                "ticker = %4d",
                data->state[3],
                data->send_charakter,
                data->accepted_charakter,
                data->counter,
                data->preriod);
            fflush(stdout);
        }

        // Ukončení vlákna
        q = data->quit;
        pthread_mutex_unlock(&mtx);
    }
    
    // Podmínkové ukončení seriové linky
    if (data->nukleo != -1)
    {
        serial_close(data->nukleo);
    }
    
    return 0;
}

void call_termios(int reset)
{
    static struct termios tio, tioOld; // use static to preserve the initial settings
    tcgetattr(STDIN_FILENO, &tio);
    if (reset)
    {
        tcsetattr(STDIN_FILENO, TCSANOW, &tioOld);
    } else {
        tioOld = tio; //backup
        cfmakeraw(&tio);
        tcsetattr(STDIN_FILENO, TCSANOW, &tio);
    }
}

void* inputThread(void *v) 
{
    
    // Založení globálních struktury do fce
    SharedData* data = (SharedData*) v;
    bool q = false;
    bool qq = false;
    
    // Čekání než se načte zařízení z fce Output
    while (!qq && !q) 
    {
        pthread_mutex_lock(&mtx);
        qq = data->is_serial_open;
        q = data->quit;
        pthread_mutex_unlock(&mtx);
    }
    
    // Předávání znaků z klávsnice do nuklea 
    while (!q)
    {
        // načtení znaků z klávesnice
        char c = getchar();

        // Zabezpečení vlákna
        pthread_mutex_lock(&mtx);
        
        // Kontrola povolených znaků k ovládání nuklea
        if (c == 'q' || c == 'h' || c == 'b' || c == 's' ||c == 'e' || (c >= '1' && c <= '5'))
        {
            // Uložení předaného znaku z klavesnice do nuklea
            data->send_charakter = c;

            //Ukončení programu pomocí dvou znaků 
            if (c == 'q' )
            {
                q = true;
                data->quit = q;
            }

            // Komunikace s nukleme pomoci predem pripravene fce 
            int r = serial_putc(data->nukleo, c);

            // Overeni mozne chyby v komunikaci s nukleem 
            if (r == -1) 
            {
                fprintf(stderr, "ERROR, input char");
                q = true;
                data->quit = q;
            }
        } else {
            data->send_charakter = '?';
        }
        pthread_mutex_unlock(&mtx);
    }
    
    return 0;
}
