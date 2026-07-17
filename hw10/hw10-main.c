#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include <assert.h>

#include <termios.h> 
#include <unistd.h>  // for STDIN_FILENO

#include <pthread.h>

#include "prg_serial_nonblock.c"
#include "messages.c"
#include "event_queue.c"//

#define SERIAL_READ_TIMOUT_MS 500 //timeout for reading from serial port

// shared data structure
typedef struct {
   bool quit; // proměná ukončení vláken
   int fd; // serial port file descriptor
} data_t;

pthread_mutex_t mtx;
pthread_cond_t cond;

void call_termios(int reset); // aktivace raw terminálu

void* input_thread(void*);
void* serial_rx_thread(void*); // serial receive buffer

bool send_message(data_t *data, message *msg); // pošle data nukleu ??

// hlavní vlákno na zpracování dat
int main(int argc, char *argv[])
{
   // inicializace sdílených dat
   data_t data = { .quit = false, .fd = -1};
   const char *serial = argc > 1 ? argv[1] : "/dev/ttyS3";
   
   // otevření serového portu
   data.fd = serial_open(serial);

   // kontrola otevření serial portu
   if (data.fd == -1) {
      fprintf(stderr, "ERROR: Cannot open serial port %s\n", serial);
      exit(100);
   }

   // Zadefinování počtu a jmen vláken
   enum { INPUT, SERIAL_RX, NUM_THREADS };
   const char *threads_names[] = { "Input", "Serial In" };

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

   // struktura pro zaznamenání výpočtu 
   struct {
      uint16_t chunk_id;
      uint16_t nbr_tasks;
      uint16_t task_id;
      bool computing;
   } computation = { 0, 0, 0, false };
   message msg;
   
   _Bool quit = false; // kontrolní proměná k ukončení vlákna
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
               }
               break;

            // příkaz pro nukleo: pošli výpočty a počet kolikrát máš počítat 
            case EV_COMPUTE:
               {
                  msg.type = MSG_COMPUTE; // zadání určité zprávy k zadání příkazu v nukleu
                  computation.computing = true; // označení stavu, že počítač počítá

                  // počet task_id, které má počítač udělat
                  computation.nbr_tasks = 10*(ev.data.param - '0'); 
                 
                  // +1 k příkazu počítání a zadaných do struktury k předání nukleu
                  msg.data.compute.chunk_id = ++(computation.chunk_id); 
                  
                  // počet task_id, které má počítač udělat zadaných do struktury k předání nukleu
                  msg.data.compute.nbr_tasks = 10*(ev.data.param - '0'); 
                  fprintf(stderr, "INFO: Set the calculate parametrs for nukleo\r\n");
                  fprintf(stdout, "%d - nbr_tasks, %d - chunk_id, %d - task_id\r\n",
                     computation.nbr_tasks,computation.chunk_id,computation.task_id);
               }
               break;

            // příkaz od uživatele k resetování čísla výpočtu 
            // za předpokladu,že nukleo již nepočítá
            case EV_RESET_CHUNK:
               {
                  if (computation.computing == false)
                  {
                     computation.chunk_id = 0;
                     fprintf(stderr, "INFO: Get reset chunk_id\r\n");
                  } else {
                     fprintf(stderr, "WARN: Cannot reset chunk_id, when nulkeo still calculate\r\n");
                  }
               }
               break;   

            // příkaz pro nukleo k ukončení výpočtu 
            case EV_ABORT:
               msg.type = MSG_ABORT;
               computation.computing = false;
               fprintf(stderr, "INFO: Get abort nukleo and boss program\r\n");
               break;
               
            // příkaz na ukončení obou programů
            case EV_QUIT:
               fprintf(stderr, "INFO: Quit whole program\r\n");
               pthread_mutex_lock(&mtx);
               data.quit = true; // quit whole program
               quit = data.quit;
               pthread_mutex_unlock(&mtx);
               break;
            
            // - Když se zmáčkne cokoliv jiného než controlní znaky, 
            //   tak se vytiskne info o znacích používaných ke kontrole programu
            case EV_INFO: 
               fprintf(stdout, "INFO: Control buttons are: 'g' - for get version,"
                               "'1' -> '5' - for senf nbr_task to nukleo," 
                               "'a' - for abort calculation on nukleo,"
                                "'r' - for reset chunk_id,"
                               " 'q' - for abort both programs\r\n");
               break;

            default: // discard all unnecessary functions
               fprintf(stderr, "DEBUG: user or program send unknown messages, please check" 
                  "still once the both of programs!\r\n");
               break;
         }
         if (msg.type != MSG_NBR) { // messge has been set
            if (!send_message(&data, &msg)) {
               fprintf(stderr, "ERROR: send_message() does not send all bytes of the message!\r\n");
            } else {
               fprintf(stdout, "INFO: The message was sended to the nukleo\r\n");
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
                  }
                  break;
               
               // hláška od nuklea, s informacemi k verzi daného programu 
               case MSG_VERSION:
                  if (msg->data.version.patch > 0) {
                     fprintf(stderr, "INFO: Nucleo firmware ver. %d.%d-p%d\r\n", 
                     msg->data.version.major, msg->data.version.minor, msg->data.version.patch);
                  } else {
                     fprintf(stderr, "INFO: Nucleo firmware ver. %d.%d\r\n", 
                     msg->data.version.major, msg->data.version.minor);
                  }
                  break;

               // zpráva od nuklea, které poslalo potvrzující hlášku o tom,
               // že vše proběhlo v pořádku
               case MSG_OK:
                  // hláška při zasílání zpráva o zahájení výpočtu 
                  if (computation.computing == true)
                  { 
                     fprintf(stderr, "INFO: Calculation on nukleo start correctly\r\n");
                  } else { // hláska o korekním ukončení výpočtu na nukleu
                     fprintf(stderr, "INFO: Calculation was stopped correctly\r\n");
                  }
                  break;

               // Zpráva od nuklea, které poslalo chybovou hlášku
               // při zpracování příkazu od řídícího porgramu 
               case MSG_ERROR:
                  // hláška od nuklea, že výpočet nemůže v tuto chvíli začít  
                  if (computation.computing == true)
                  {
                     fprintf(stderr, "WARN: Calculation cannot start\r\n");
                     
                  } else { // hláška od nukle, že výpočet nebyl řádně ukončen
                     fprintf(stderr, "WARN: Calculation wasn't stopped correctly\r\n");
                  }
                  break;

               // zpráva od nuklea, která se týká ukončení výpočtu skrze tlačítko
               case MSG_ABORT:
                  fprintf(stderr, "INFO: Nukleo's button stopped calculation on Nukleu\r\n");
                  break;

               // zpráva od nuklea, které poslalo data s výpočty
               case MSG_COMPUTE_DATA:
                  fprintf(stderr, "INFO: New data chunk id: %d, task id: %d - results %d\r\n",
                   msg->data.compute_data.chunk_id, msg->data.compute_data.task_id,
                    msg->data.compute_data.result);
                  break;
               
               // zpráva od nuklea, které poslalo zprávu o dokončení výpočtu
               case MSG_DONE:
                  computation.computing = false;
                  fprintf(stderr, "INFO: Computation on nukleo was ended correctly\r\n");
                  break;

               // Tato akce je to tam jenom pro upozornění na chybu v programu,
               // jelikož tento případ nemůže nikdy nastat 
               default:
                  fprintf(stderr, "DEBUG: Nukleo send unknown messages, please check" 
                  "still once the both of programs!\r\n");
                  break;
            }
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
   
   // - zde je tohle jenom, protože nás zajímá odkud ty data přišli a chceme dostat tuto informaci
   event ev = { .source = EV_KEYBOARD };
   while ( !end && (c = getchar())) {
          ev.type = EV_TYPE_NUM;
      switch(c) {

         case 'g': // get version
             ev.type = EV_GET_VERSION;//
             // - tohle jenom stačí k ovládání?
            break;

         case 'r': // reset chunk_id
            ev.type = EV_RESET_CHUNK;
            break;

         case 'a': // abort nukleo
            ev.type = EV_ABORT;
            break;

         case '1': // Compute some calculation 
            ev.type = EV_COMPUTE;
            ev.data.param = '1';
            break;
         case '2': // Compute some calculation 
            ev.type = EV_COMPUTE;
            ev.data.param = '2';
            break;
         case '3': // Compute some calculation 
            ev.type = EV_COMPUTE;
            ev.data.param = '3';
            break;
         case '4': // Compute some calculation 
            ev.type = EV_COMPUTE;
            ev.data.param = '4';
            break;
         case '5': // Compute some calculation 
            ev.type = EV_COMPUTE;
            ev.data.param = '5';
            break;

         case 'q': // quit whole program
            ev.type = EV_QUIT;
            end = true;
            break;

         default: // discard all other keys
            ev.type = EV_INFO;
            break;
      }

      // - Kontrolní podmínka, pokud se načetla zpráva z klávesnice do počítače kvůli tomu,
      // že se vláknu bude pokoušet stále něco načítat, tak aby se nám nezaplnila okamžitě fronta
      if (ev.type != EV_TYPE_NUM) { // new event //
         queue_push(ev);//
      }// else no akcio was send to do 

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
   int size = 0; // - velikost bufferu

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
               fprintf(stderr, "INFO: Data was processed and distributed\r\n");
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
