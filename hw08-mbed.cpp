#include "mbed.h"

#define LEVEL_SPEED1 1
#define LEVEL_SPEED2 0.5
#define LEVEL_SPEED3 0.4
#define LEVEL_SPEED4 0.3
#define LEVEL_SPEED5 0.2
#define LEVEL_SPEED6 0.1
#define LEVEL_SPEED7 0.05 

DigitalOut myled(LED1); // Ovládán tlačítka 
InterruptIn myBtn(USER_BUTTON); // Zaznamenání stiku tlačítka
DigitalIn btnPress(USER_BUTTON); // Zasnamenání stisku tlačítka
Ticker ledTicker; // Časovač pro intervali blikání LED diody
Ticker btnPressTicker; // Časovač pro zjištění délky stisknutého tlačítka 
Ticker btnTicker; // Časovač pro neinterakci s tlačítkem  
int time_run = 0; // Counter délka stisknutí tlačítka
int Step_speed = 1; // Stupěň rychlosti
bool start_set = true; // Počátečný chod LED1 nastaven na LEVEL_SPEED1
bool pressIgnore = false; // kontrolní hodnota pro ustálení stavu

// Rozsvěcení a zhasínání LED doidy
void flip()
{
    myled = !myled;
}

// Počítadlo délky stisku tlačítka a následná interakce
void controlBtn()
{
    // - Pokud-li se tlačítko uvolní, tak přenastavit Step_speed
    // a zrušit vracení se do této fce
    if (btnPress == 1)
    {
        btnPressTicker.detach();
        Step_speed += 1;
    }
    time_run++;
    
    // - Pokud-li je naměření 200 nebo více návratů do této fce,
    // tak přehodit Step_speed = 1 a zrušit vracení se do této fce
    if (time_run >= 200)
    {
        ledTicker.attach(&flip, LEVEL_SPEED1);
        Step_speed = 1;        
        btnPressTicker.detach();      
    }
}

// Kontrola ustáleného signálu a délky stisknutí tlačítka
void clearIgnore()
{
    time_run = 0;
    btnPressTicker.attach(&controlBtn, 0.001);
    pressIgnore = false;
    btnTicker.detach();    
}

// Přenastavování stavů rychlosti blikání na LED diodě
void pressed()
{
    // - Kontrolní podmínka na zamezení ovládání tlačítka,
    // když se rorovnává signál
    if (!pressIgnore)
    {
        if (Step_speed == 2)
        {
            ledTicker.attach(&flip,LEVEL_SPEED2);
        }
        if (Step_speed == 3)
        {
            ledTicker.attach(&flip,LEVEL_SPEED3);
        }
        if (Step_speed == 4)
        {
            ledTicker.attach(&flip,LEVEL_SPEED4);
        }
        if (Step_speed == 5)
        {
            ledTicker.attach(&flip,LEVEL_SPEED5);
        }
        if (Step_speed == 6)
        {
            ledTicker.attach(&flip,LEVEL_SPEED6);
        }
        if (Step_speed == 7)
        {
            ledTicker.attach(&flip,LEVEL_SPEED7);
        } 
        if (Step_speed > 7)
        {
            ledTicker.detach();
            myled = 1;
        }
               
        // - pozastavení ovládání LED diody z důvodu neustáleného signálu 
        btnTicker.attach(&clearIgnore, 0.15);
        pressIgnore = true;
    }
}


int main()
{
    // - Počáteční stav blikání LED diody
    // - Poté neaktivní
    if (start_set == true)
    {
        ledTicker.attach(&flip, LEVEL_SPEED1);
        start_set = false;
    }
    // - Interakce a zaznamenání stisknutí tlačítka     
    myBtn.fall(&pressed);
}