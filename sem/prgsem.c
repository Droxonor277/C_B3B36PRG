#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include <assert.h>

#include <termios.h> 
#include <unistd.h>  // for STDIN_FILENO

#include <pthread.h>

#include "prg_serial_nonblock.h"
#include "messages.h"
#include "event_queue.h"
#include "xwin_sdl.h"
#include "math.h"
#include "function.h"
#include <time.h>

#define SERIAL_READ_TIMOUT_MS 500 //timeout for reading from serial port

// shared data structure
typedef struct {
   bool quit; // proměná ukončení vláken
   int fd; // serial port file descriptor
   bool pause; // pozastavit načítání znaků na danou dobu
   bool computing; // kontrolní proměná na kontrolu jestli se spustil výpočet
   bool stop_loading; //kontrolní proměná na poyastavení načítání znaků vlákna 
   bool abort; // kontrolní proměná, jeslti je přerušen výpočet;
   bool animation; // kontrolní proměná, detekující, jeslti je spuštěná animace
} data_t;

pthread_mutex_t mtx;
pthread_cond_t cond;

void call_termios(int reset); // aktivace raw terminálu

void* input_thread(void*); // vlákno pro vstup kontrolních klíčů
void* serial_rx_thread(void*); // serial receive buffer

bool send_message(data_t *data, message *msg); // pošle data nukleu 

// hlavní vlákno na zpracování dat
int main(int argc, char *argv[])
{
   // inicializace sdílených dat
   data_t data = { .quit = false, .fd = -1, .pause = false,
                   .computing = false, .stop_loading = false, 
                   .abort = false, .animation = false};
   const char *serial = argc > 1 ? argv[1] : "/dev/ttyACM0";
   
   // otevření serového portu
   data.fd = serial_open(serial);

   // kontrola otevření serial portu
   if (data.fd == -1) {
      fprintf(stderr, "ERROR: Cannot open serial port %s\n", serial);
      exit(100);
   }

   // Zadefinování počtu a jmen vláken
   enum { INPUT, SERIAL_RX, NUM_THREADS };
   const char *threads_names[] = { "Input", "Serial In"};

   // vytvoření požadovaných vláken
   void* (*thr_functions[])(void*) = { input_thread, serial_rx_thread};

   pthread_t threads[NUM_THREADS]; 
   pthread_mutex_init(&mtx, NULL); // initialize mutex with default attributes
   pthread_cond_init(&cond, NULL); // initialize mutex with default attributes

   call_termios(0); // aktivace raw modu pro termilná

   // Alokace vláken
   for (int i = 0; i < NUM_THREADS; ++i) {
      int r = pthread_create(&threads[i], NULL, thr_functions[i], &data);
      fprintf(stderr, "INFO: Create thread '%s' %s\n", threads_names[i], ( r == 0 ? "OK" : "FAIL") );
   }

   message msg;

   computation_init();
   gui_init();
   _Bool quit = false; // kontrolní proměná k ukončení vlákna
   print_entered_data();
   print_start();
   while (!quit) {

      // výstup první zprávy v kruhové frontě a daný jí ke zpracování
      event ev = queue_pop();

      // správa zpráv od uživatele
      if (ev.source == EV_KEYBOARD) {
         msg.type = MSG_NBR;
         
         switch(ev.type) {

            //příkaz pro nukleo: pošli verzi systému
            case EV_GET_VERSION:
               { // prepare packet for get version
                  msg.type = MSG_GET_VERSION; // nastavení typu pr
                  fprintf(stderr, "INFO: Get version requested\r\n");
                  fflush(stderr);
               }
               break;

            // příkaz pro nukleo: pošli výpočty a počet kolikrát máš počítat 
            case EV_COMPUTE:
                     if (compute(&msg))
                     {
                        enable_comp();
                        pthread_mutex_lock(&mtx);
                        data.abort = false;
                        data.computing = true; 
                        pthread_mutex_unlock(&mtx);
                        fprintf(stdout,"INFO: Compute was started\r\n");
                        fflush(stdout);
                     } else {
                        fprintf(stdout,"ERROR: Compute failed!\r\n");
                        fflush(stdout);
                     }
               break;

            // příkaz od uživatele k resetování výpočtu 
            // za předpokladu,že nukleo již něco spočítalo, protože jinak je to zbytečné
            case EV_RESET:
               restart_compute();
               fprintf(stdout, "INFO: Set compute was sucesfull\r\n");
               fflush(stdout);
               break;   
            
            // nastavý data k výpočtu na nukleu
            case EV_SET_COMPUTE:
                  if (set_compute(&msg))
                  {
                     fprintf(stdout, "INFO: Set compute was sucesfull\r\n");
                     fflush(stdout);
                     
                  } else {
                     fprintf(stderr, "ERROR: Set_compute failed\r\n");
                     fprintf(stdout, "INFO: You can press 'r' for restart"
                                    " calculation and again set compute data\r\n");
                     fflush(stdout);
                  }
               break;

            // zobrazí spočítána data do obrázku z pokynu uživatele
            case EV_REFRESH:
               gui_refresh();
               xwin_poll_events();
               pthread_mutex_lock(&mtx);
               pthread_cond_signal(&cond);
               pthread_mutex_unlock(&mtx);
               fprintf(stdout, "INFO: You refesh buffer to display\r\n");
               fflush(stdout);
               break;
            
            // Vymaže spočítaná data
            case EV_DELETE_BUFFER:
               gui_black_screen();
               fprintf(stdout, "INFO: You delete the whole content of the buffer\r\n");
               fflush(stdout);
               break;

            // Příkaz od uživatele pro změnu parametrů konstatnty C
            case EV_LOAD_DATA_CRE:
            
               call_termios(1); // ukončení raw terminálu pro zadání nových parametrů

               // načítá zprávy ze standartního vstupu a udává info o zadaných hodnotách
               print_C();
               fprintf(stdout,"Please enter parameters c_re:");
               fflush(stdout);
               load_cre();
               fprintf(stdout,"Please enter parameters c_im:");
               fflush(stdout);  
               load_cim();             
               print_C();
               
               call_termios(0); // obnovení raw terminálu

               pthread_mutex_lock(&mtx);
               data.stop_loading = false; // ukončení zastavení načítání znaků 
               data.pause = false; // ukončení načítání parametrů vstupu
               pthread_mutex_unlock(&mtx);
               break;

            // Informování uživatele, že zadal špatný control key při načítání parametrů
            case EV_SET_WRONG_KEY:
               fprintf(stdout,"You set wrong key and setting parametres was ended\r\n");
               fflush(stdout);  
               pthread_mutex_lock(&mtx);
               data.stop_loading = false; // ukončení zastavení načítání znaků 
               data.pause = false; // ukončení načítání parametrů vstupu
               pthread_mutex_unlock(&mtx);               
               break;

            // Příkaz od uživatele pro uložení načteného obrázku
            case EV_SAVE_IMAGE:
               save_image();
               fprintf(stdout,"INFO: The image was saved as a .ppm file "
                              "into the folder where is boss program\r\n");
               fflush(stdout);
               break;
            
            // Příkaz od uživatele o zahájení animace
            case EV_ANIMATION:
               fprintf(stdout,"INFO: Animation was started!\r\n");
               fflush(stdout);
               fprintf(stdout,"====================================================================\n"
                              "| INFO: Now available control keys to control animation            |\n"
                              "|==================================================================|\n"
                              "| 'h' | to stop the animation                                      |\n"
                              "|=====|============================================================|\n"
                              "| 'm' | to set control variable and we can continue the animation  |\n"
                              "|=====|============================================================|\n"
                              "| 'n' | to change animation on oposite way                         |\n"
                              "|=====|============================================================|\n"
                              "| '/' | to increase imaginary shift by set value                   |\n"
                              "|=====|============================================================|\n"
                              "| '*' | to decrease imaginary shift by set value                   |\n"
                              "|=====|============================================================|\n"
                              "| '-' | to decrease real shift by set value                        |\n"
                              "|=====|============================================================|\n"
                              "| '+' | to increase real shift by set value                        |\n"
                              "|=====|============================================================|\n"
                              "| Any other control key causes stop animation and again will       |\n"
                              "| be available all control keys                                    |\n"
                              "===================================================================\n");
               fflush(stdout);
               animation_image();
               break;

            // výpiše na standartní výstup všechna data na příkaz uživatele
            case EV_PRINT_DATA:
               print_entered_data();
               break;

            // výpočet fraktátu na počítači na příkaz uživatele
            case EV_COMPUTE_CPU:
               fprintf(stdout, "INFO: CPU start compute Julian set on PC\r\n");
               compute_Julian_set();
               break;

            // Z příkazu uživatele se změní velikost imaginárního intervalu 
            case EV_CHANGE_DIMENSIONS_IM:
               call_termios(1); // ukončení raw terminálu pro zadání nových parametrů
               print_im_interval_values();
               fprintf(stdout,"Please enter the minimum value of the imaginary interval component:");
               fflush(stdout);                 
               loadmin_im();
               fprintf(stdout,"Please enter the maximum value of the imaginary interval component:");
               fflush(stdout);  
               loadmax_im();
               comp_d_im();
               print_im_interval_values();              
               call_termios(0); // obnovení raw terminálu
               pthread_mutex_lock(&mtx);
               data.stop_loading = false; // ukončení zastavení načítání znaků 
               data.pause = false; // ukončení načítání parametrů vstupu
               pthread_mutex_unlock(&mtx); 
               break;

            // příkaz od uživatele k načtení ze standartního vstupu počet iterací 
            case EV_CHANGE_NUMBER_ITERATION:
               call_termios(1); // ukončení raw terminálu pro zadání nových parametrů
               print_number_iteration();
               fprintf(stdout,"Please enter the new number of iteration on every pixel:");
               fflush(stdout); 
               load_number_iteration();
               print_number_iteration();
               call_termios(0); // obnovení raw terminálu
               pthread_mutex_lock(&mtx);
               data.stop_loading = false; // ukončení zastavení načítání znaků 
               data.pause = false; // ukončení načítání parametrů vstupu
               pthread_mutex_unlock(&mtx);                
               break;
            
            // Načtení nové reálné části shiftu posunutí
            case EV_CHANCE_SHIFT_ANIME_RE:
               call_termios(1); // ukončení raw terminálu pro zadání nových parametrů
               print_animation_change_shift();
               fprintf(stdout,"Please enter the new real part of shift for animation:");
               fflush(stdout); 
               load_new_re_shift();
               print_animation_change_shift();
               call_termios(0); // obnovení raw terminálu
               pthread_mutex_lock(&mtx);
               data.stop_loading = false; // ukončení zastavení načítání znaků 
               data.pause = false; // ukončení načítání parametrů vstupu
               pthread_mutex_unlock(&mtx);   
               break;

            // Načtení nové imaginární části shiftu posunutí
            case EV_CHANCE_SHIFT_ANIME_IM:
               call_termios(1); // ukončení raw terminálu pro zadání nových parametrů
               print_animation_change_shift();
               fprintf(stdout,"Please enter the new imaginary part of shift for animation:");
               fflush(stdout); 
               load_new_im_shift();
               print_animation_change_shift();
               call_termios(0); // obnovení raw terminálu
               pthread_mutex_lock(&mtx);
               data.stop_loading = false; // ukončení zastavení načítání znaků 
               data.pause = false; // ukončení načítání parametrů vstupu
               pthread_mutex_unlock(&mtx);
               break;

            // Z příkazu uživatele se změní velikost reálného intervalu 
            case EV_CHANGE_DIMENSIONS_RE:
               call_termios(1); // ukončení raw terminálu pro zadání nových parametrů
               print_re_interval_values();
               fprintf(stdout,"Please enter the minimum value of the real interval component:");
               fflush(stdout);  
               loadminre();
               fprintf(stdout,"Please enter the maximum value of the real interval component:");
               fflush(stdout); 
               load_max_re();
               comp_d_re();
               print_re_interval_values();
               call_termios(0); // obnovení raw terminálu
               pthread_mutex_lock(&mtx);
               data.stop_loading = false; // ukončení zastavení načítání znaků 
               data.pause = false; // ukončení načítání parametrů vstupu
               pthread_mutex_unlock(&mtx); 
               break;

            // příkaz pro nukleo k ukončení výpočtu 
            case EV_ABORT:
               msg.type = MSG_ABORT;
               pthread_mutex_lock(&mtx);
               data.abort = true;
               pthread_mutex_unlock(&mtx);
               fprintf(stderr, "INFO: Get abort nukleo calculation\r\n");
               fflush(stdout);
               break;

            case EV_CHANGE_ANIME_RE:
               call_termios(1); // ukončení raw terminálu pro zadání nových parametrů
               print_animation_current_shirt();
               fprintf(stdout,"Please enter the whole animation shift_re:");
               fflush(stdout);                
               load_whole_new_shift_re();
               print_animation_current_shirt();
               call_termios(0); // obnovení raw terminálu
               pthread_mutex_lock(&mtx);
               data.stop_loading = false; // ukončení zastavení načítání znaků 
               data.pause = false; // ukončení načítání parametrů vstupu
               pthread_mutex_unlock(&mtx);                
               break;

            case EV_CHANGE_ANIME_IM:
               call_termios(1); // ukončení raw terminálu pro zadání nových parametrů
               print_animation_current_shirt();
               fprintf(stdout,"Please enter the whole animation shift_im:");
               fflush(stdout); 
               load_whole_new_shift_im();
               print_animation_current_shirt();
               call_termios(0); // obnovení raw terminálu
               pthread_mutex_lock(&mtx);
               data.stop_loading = false; // ukončení zastavení načítání znaků 
               data.pause = false; // ukončení načítání parametrů vstupu
               pthread_mutex_unlock(&mtx);  
               break;

            // příkaz na ukončení obou programů
            case EV_QUIT:
               fprintf(stderr, "INFO: Quit whole program\r\n");
               fflush(stdout);
               pthread_mutex_lock(&mtx);
               data.quit = true; // quit whole program
               quit = data.quit;
               pthread_cond_signal(&cond);
               pthread_mutex_unlock(&mtx);
               break;

            // Příkaz od uživatele pro informaci uživatee o jeho možnostech
            case EV_SET_DATA:
               fprintf(stdout,"=============================================================\n"
                              "| INFO: Available control keys to change set data           |\n"
                              "|===========================================================|\n"
                              "| '2' | for change the whole shift_re                       |\n"
                              "|=====|=====================================================|\n"
                              "| '3' | for change the whole shift_im                       |\n"
                              "|=====|=====================================================|\n"
                              "| '4' | for change the animation shift_re                   |\n"
                              "|=====|=====================================================|\n"
                              "| '5' | for change the animation shift_im                   |\n"
                              "|=====|=====================================================|\n"
                              "| '6' | for change the number of iterations on every pixel  |\n"
                              "|=====|=====================================================|\n"
                              "| '7' | for change the parameters on the imagiry axis       |\n"
                              "|=====|=====================================================|\n"
                              "| '8' | for change the parameters on the real axis          |\n"
                              "|=====|=====================================================|\n"
                              "| '9' | for change the parameters of constat C              |\n"
                              "|=====|=====================================================|\n"
                              "| Any other control key causes stop animation and again will|\n"
                              "| be available all control keys                             |\n"
                              "=============================================================\n");
               fflush(stdout);
               break;
            
            // - Když se zmáčkne cokoliv jiného než controlní znaky, 
            //   tak se vytiskne info o znacích používaných ke kontrole programu
            case EV_INFO: 
               print_control_key();
               break;

            default: // discard all unnecessary functions
               fprintf(stderr, "DEBUG: user or program send unknown messages, please check" 
                  "still once the both of programs!\r\n");
               fflush(stderr);
               break;
         }
         if (msg.type != MSG_NBR) { // messge has been set
            if (!send_message(&data, &msg)) {
               fprintf(stderr, "ERROR: send_message() does not send all bytes of the message!\r\n");
               fflush(stderr);
            } else {
               fprintf(stdout, "INFO: The message was sended to the nukleo\r\n");
               fflush(stdout);
            }
         }








      

      // správa zpráv od příchozích od nuklea
      } else if (ev.source == EV_NUCLEO) { 
         if (ev.type == EV_SERIAL) {

            // předání odkazu do fce jako odkaz na místo vytvořené dynamicky
            message *msg = ev.data.msg;
            switch (msg->type) {
               
               // zpráva od nukle o přihlášení s uvítací zprávou  'PRG-LAB10'
               case MSG_STARTUP:
                  {
                     char str[STARTUP_MSG_LEN+1];
                     for (int i = 0; i < STARTUP_MSG_LEN; ++i) {
                        str[i] = msg->data.startup.message[i];
                     }
                     str[STARTUP_MSG_LEN] = '\0';
                     fprintf(stderr, "INFO: Nucleo restarted - '%s'\r\n", str);
                     fflush(stderr);
                  }
                  break;
               
               // hláška od nuklea, s informacemi k verzi daného programu 
               case MSG_VERSION:
                  if (msg->data.version.patch > 0) {
                     fprintf(stderr, "INFO: Nucleo firmware ver. %d.%d-p%d\r\n", 
                     msg->data.version.major, msg->data.version.minor, msg->data.version.patch);
                     fflush(stderr);
                  } else {
                     fprintf(stderr, "INFO: Nucleo firmware ver. %d.%d\r\n", 
                     msg->data.version.major, msg->data.version.minor);
                     fflush(stderr);
                  }
                  break;

               // zpráva od nuklea, které poslalo potvrzující hlášku o tom,
               // že vše proběhlo v pořádku
               case MSG_OK:

                  if (is_set_data())
                  {
                     fprintf(stdout, "INFO: Data on nukleo was set correctly\r\n");
                     fflush(stdout);
                  } 
                  if (is_computing())
                  { 
                     fprintf(stdout, "INFO: Calculation on nukleo start correctly\r\n");
                     fflush(stdout);
                  } 
                  
                  pthread_mutex_lock(&mtx);
                  if (data.abort) { // hláska o korekním ukončení výpočtu na nukleu
                     fprintf(stdout, "INFO: Calculation was stopped correctly\r\n");
                     fflush(stdout);
                  } 
                  pthread_mutex_unlock(&mtx);
                  break;

               // Zpráva od nuklea, které poslalo chybovou hlášku
               // při zpracování příkazu od řídícího porgramu 
               case MSG_ERROR:
                  fprintf(stderr, "ERROR: Nukleo program failed!\r\n");
                  fflush(stderr);
                  break;

               // zpráva od nuklea, která se týká ukončení výpočtu skrze tlačítko
               case MSG_ABORT:
                  abort_comp();
                  fprintf(stdout, "INFO: Nukleo's button stopped calculation on Nukleu\r\n");
                  fflush(stdout);
                  break;

               // zpráva od nuklea, které poslalo data s výpočty
               case MSG_COMPUTE_DATA:
                  if (!is_aborted())
                  {
                     update_data(&(msg->data.compute_data));
                  }
                  break;
                  
               // zpráva od nuklea, které poslalo zprávu o dokončení výpočtu
               case MSG_DONE:

                  gui_refresh();  
                  if (is_done())
                  {
                     fprintf(stdout, "INFO: Computation is done!\r\n");
                     fflush(stdout);

                  } else {
                     
                     event ev = {.source = EV_KEYBOARD, .type = EV_COMPUTE};
                     queue_push(ev);
                     print_info_chunk();
                  }
                  break;

               // Tato akce je to tam jenom pro upozornění na chybu v programu,
               // jelikož tento případ nemůže nikdy nastat 
               default:
                  fprintf(stderr, "DEBUG: Nukleo send unknown messages, please check" 
                  "still once the both of programs!\r\n");
                  fflush(stderr);
                  break;
            } // end switch
     
      
            // Pokudli je něco v msg, tak vyprázdnit alokování data v této struktuře
            if (msg) {
               free(msg);
            }

         // zpráva od nuklea, které poslalo příkaz o ukonření všech procesů
         } else if (ev.type == EV_QUIT) {
            quit = true;
         } else {
            // ignore all other events
         }
      }
   } // end main quit

   // uvolní vypočítaný obrázek
   gui_cleanup();

   // Odstraní všechny data používaná k výpočtu
   computation_cleanup();









   queue_cleanup(); // cleanup all events and free allocated memory for messages. // 
   
   // ukončení všech vláken a uzavření seriové linky a ukončení raw terminálu 
   for (int i = 0; i < NUM_THREADS; ++i) {
      fprintf(stderr, "INFO: Call join to the thread %s\r\n", threads_names[i]);
      int r = pthread_join(threads[i], NULL);
      fprintf(stderr, "INFO: Joining the thread %s has been %s\r\n",
       threads_names[i], (r == 0 ? "OK" : "FAIL"));
   }
   
   serial_close(data.fd); // zavření sériového portu
   call_termios(1); // restore terminal settings
   return EXIT_SUCCESS;
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


















// Vlákno přijímající znaky z klávesnice, díky kterým udává pokyny skrze kruhovou frontu
void* input_thread(void* d)
{
   // zavedení sdílené struktury
   data_t *data = (data_t*)d;
   bool end = false; // kontrolní proměná na ukončení vlákna
   int c; // zavedení předávaného znaku 
   // bool t = true;
   
   // - zde je tohle jenom, protože nás zajímá odkud ty data přišli a chceme dostat tuto informaci
   event ev = { .source = EV_KEYBOARD };
   while ( !end && (c = getchar())) {
      ev.type = EV_TYPE_NUM;
      
      if (data->pause == false && data->animation == false)
      {
         switch(c) {

            case 'g': // get version
               ev.type = EV_GET_VERSION;
               break;

            case 'r': // reset chunk_id
               ev.type = EV_RESET;
               break;

            case 'a': // abort nukleo
               ev.type = EV_ABORT;
               break;

            case '1': // Compute some calculation 
               ev.type = EV_COMPUTE;
               break;

            case 'i': // get info
               ev.type = EV_INFO;
               break;

            case 's': // set the computer parametrs
               ev.type = EV_SET_COMPUTE;
               break;

            case 'l': // delete data store in buffer at now
               ev.type = EV_DELETE_BUFFER;
               break;

            case 'f': // animation image
               ev.type = EV_ANIMATION;
               pthread_mutex_lock(&mtx);
               data->animation = true;
               pthread_mutex_unlock(&mtx);
               break;

            case 'o': // print all data
               ev.type = EV_PRINT_DATA;
               break;
            

            case 'p': // redraw into picture with data stored in buffer at now
               ev.type = EV_REFRESH;
               break;

            case 'u': // control key for next option to set key
               ev.type = EV_SET_DATA;
          
               pthread_mutex_lock(&mtx);
               data->pause = true;
               pthread_mutex_unlock(&mtx);
               break;

            // uloží obrázek do složky ve které se nachází kód a ve formátu .ppm
            case 't':
               ev.type = EV_SAVE_IMAGE;
               break;

            case 'c': // Calkulate fractal in pc and show that
               ev.type = EV_COMPUTE_CPU;
               break;


            case 'q': // quit whole program
               ev.type = EV_QUIT;
               end = true;
               break;
            
            case '\n':
               // discard enter key
               break;

            case 'h': // stop animation
               stop_animation();
               fprintf(stdout,"INFO: Animation was stopped\r\n");
               fflush(stdout);
               break;

            // přičtení reálné části shiftu od peoridického reálného posunutí konstanty C 
            case '+': // plus shift data
               plus_shift_re();
               print_animation_current_shirt();
               break;

            // odečtení reálné části shiftu od peoridického reálného posunutí konstanty C 
            case '-': // minut real shift data
               minus_shift_re();
               print_animation_current_shirt();
               break;

            // přičtení imaginární části shiftu od peoridického imaginárního posunutí konstanty C 
            case '/': // plus imagination shift data
               plus_shift_im();
               print_animation_current_shirt();
               break;
            
            // odečtení imaginární části shiftu od peoridického imaginárního posunutí konstanty C             
            case '*': // minut imagination shift data
               minus_shift_im();
               print_animation_current_shirt();
               break;

            case 'm':
               continue_animation();
               fprintf(stdout,"INFO: You can press 'f' again and continue in animation or press 'm' again"
                              "for animation from start parameters!\n");
               fflush(stdout);
               break;
            
            case 'n':
               oposite_animation();
               fprintf(stdout,"INFO: You change animation on oposite way!\n");
               fflush(stdout);               
               break;

            default: // get info when user don't use any other control key
               ev.type = EV_INFO;
               break;
         }
      } else if (data->animation == true)
      {
         switch (c)
         {

            case 'h': // stop animation
               stop_animation();
               fprintf(stdout,"INFO: Animation was stopped\r\n");
               fflush(stdout);
               break;

            // přičtení reálné části shiftu od peoridického reálného posunutí konstanty C 
            case '+': // plus shift data
               plus_shift_re();
               print_animation_current_shirt();
               break;

            // odečtení reálné části shiftu od peoridického reálného posunutí konstanty C 
            case '-': // minut real shift data
               minus_shift_re();
               print_animation_current_shirt();
               break;

            // přičtení imaginární části shiftu od peoridického imaginárního posunutí konstanty C 
            case '/': // plus imagination shift data
               plus_shift_im();
               print_animation_current_shirt();
               break;
            
            // odečtení imaginární části shiftu od peoridického imaginárního posunutí konstanty C             
            case '*': // minut imagination shift data
               minus_shift_im();
               print_animation_current_shirt();
               break;

            case 'm':
               continue_animation();
               fprintf(stdout,"INFO: You can press 'f' again and continue in animation or press 'm' again"
                              "for animation from start parameters!\n");
               fflush(stdout);
               break;
            
            case 'n':
               oposite_animation();
               fprintf(stdout,"INFO: You change animation on oposite way!\n");
               fflush(stdout);               
               break;

            default: // get info when user don't use any other control key
               stop_animation();
               fprintf(stdout,"INFO: Animation was stopped\r\n");
               fflush(stdout);
               fprintf(stderr,"WARN: User press wrong control key\r\n");
               fflush(stderr);
               pthread_mutex_lock(&mtx);
               data->animation = false;
               pthread_mutex_unlock(&mtx);               
               break;
         }
      } else if (data->pause == true ){

         switch(c) {

            case '9':
               ev.type = EV_LOAD_DATA_CRE;
               break;

            case '8':
               ev.type = EV_CHANGE_DIMENSIONS_RE;
               break;

            case '7':
               ev.type = EV_CHANGE_DIMENSIONS_IM;
               break;

            case '6':
               ev.type = EV_CHANGE_NUMBER_ITERATION;
               break;

            case '5':
               ev.type = EV_CHANCE_SHIFT_ANIME_RE;
               break;

            case '4':
               ev.type = EV_CHANCE_SHIFT_ANIME_IM;
               break;

            case '3':
               ev.type = EV_CHANGE_ANIME_RE;
               break;

            case '2':
               ev.type = EV_CHANGE_ANIME_IM;
               break;

            default:
               // discard all other keys
               ev.type = EV_SET_WRONG_KEY;
               fprintf(stderr,"WARN: User press wrong control key\r\n");
               fflush(stderr);
               break;
         }
         data->stop_loading = true;
      }

         // - Kontrolní podmínka, pokud se načetla zpráva z klávesnice do počítače kvůli tomu,
         // že se vláknu bude pokoušet stále něco načítat, tak aby se nám nezaplnila okamžitě fronta
         if (ev.type != EV_TYPE_NUM) { // new event //
            queue_push(ev);//
         }// else no akcion was send to do 

      // Pozastaví načítání znaků z tohoto vlákna
      while (data->stop_loading == true) { }


      pthread_mutex_lock(&mtx);
      end = end || data->quit; // check for quit
      pthread_mutex_unlock(&mtx);
   }

   // korektní ukončení programu a podání informace o tom 
   ev.type = EV_QUIT;//
   queue_push(ev);//
   fprintf(stderr, "INFO: Exit input thead %p\r\n", (void*)pthread_self());
   return NULL;
}













// Vlákno přijímá zprávy z nuklea a distribujeje do struktury, kterou následně pošle do fronty
void* serial_rx_thread(void* d) 
{ // read bytes from the serial and puts the parsed message to the queue
   data_t *data = (data_t*)d;
   uint8_t msg_buf[sizeof(message)]; // maximal buffer for all possible messages defined in messages.h
   event ev = { .source = EV_NUCLEO, .type = EV_SERIAL, .data.msg = NULL };//
   bool end = false; // kontrolní porměná na ukončení vlákna
   unsigned char c; // proměná na zadání velikosti a uložení načtené hodnoty a pomocí ní předání do bufferu
   while (serial_getc_timeout(data->fd, SERIAL_READ_TIMOUT_MS, &c) > 0) {}; // discard garbage

   bool control = false; // kontrolní poměná indikující přečtení zprávy
   int size = 0; // - velikost buffer


   while (!end) {
      int r = serial_getc_timeout(data->fd, SERIAL_READ_TIMOUT_MS, &c);
      if (r > 0) { // character has been read

         // Přečtení první ho bajtu a zjištění podle něho jeho hodnoty 
         bool control_message = get_message_size(c, &size);
         msg_buf[0] = (uint8_t) c; // uložení prvního bajtu do bufferu

         // Podmínka na zpuštění dalšího čtení ze seriového portu a přečtení zbytku paketu
         if (control_message)
         {
            
            for(int i = 0; i < size - 1;i++)
            {
               r = serial_getc_timeout(data->fd, SERIAL_READ_TIMOUT_MS, &c);
               if (r > 0)
               {
                  msg_buf[i+1] = (uint8_t) c;
               }
            }
            // alokace paměti pro vyvoření dostatečného místa pro uložení informací
            // do struktury, které jsou uloženy na odkaz msg
            message* msg = malloc(sizeof(message));
            if (msg == NULL)
            {
               fprintf(stderr,"ERROR: Low memory availabe!\r\n");
               end = true;
            }
            
            // Rozdělení bufferu do struktury message 
            if (!parse_message_buf(msg_buf, size, msg))
            {
               fprintf(stderr, "ERROR: Cannot distribute data!\r\n");
               end = true; 
            } else {
               ev.data.msg = msg; // nebo mám vrátit (event něco) a referenci na něho?
               queue_push(ev); // dání struktury na konec kruhové fronty k zpracování
               control = true; // reset a příprava na další zprávu
               size = 0; // reset a příprava na další zprávu
               //fprintf(stderr, "INFO: Data was processed and distributed\r\n");
               //start = true;
            }            
         }

      } else if (r == 0) { //read but nothing has been received
         if (control == true)
         {
            fprintf(stderr, "INFO: Message was readed\r\n");
            control = false;
         }

      } else {
         fprintf(stderr, "ERROR: Cannot receive data from the serial port\r\n");
         end = true;
      }

      // Konrola na ukončení smyčky ve vlákně 
      pthread_mutex_lock(&mtx);
      end = end || data->quit; // check for quit
      pthread_mutex_unlock(&mtx);
   } 

   ev.type = EV_QUIT; // zadání příkazu na ukončení všech ostatních vláken, když se ukončí proběh tohto
   queue_push(ev); // zařazení na konec kruhové fronty 
   fprintf(stderr, "INFO: Exit serial_rx_thread %p\r\n", (void*)pthread_self());

   return NULL;
}

// Fce připravuje a posílá data nukleu přes sériový port
bool send_message(data_t *data, message *msg) 
{
   uint8_t msg_buf[sizeof(message)]; // alokace bufferu pro uloženi marhalingem 
   int size = 0; // proměná na uložení velikosti   
   bool foo = fill_message_buf(msg, msg_buf,sizeof(message), &size); // zpracování dat pro odeslání  
   pthread_mutex_lock(&mtx);
   int ret = write(data->fd, msg_buf, size); // odeslání dat v bufferu
   pthread_mutex_unlock(&mtx);

   return ret == size && foo; // kontrolní potvrzení korektního průběhu odeslání
}

