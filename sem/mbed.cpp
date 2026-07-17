#include "mbed.h"
#include "messages.h"
#include <math.h>

Serial pc(SERIAL_TX, SERIAL_RX);
DigitalOut myled(LED1);
InterruptIn abort_btn(USER_BUTTON);

Ticker ticker_led;

bool message_abort = false; // kontrolní proměná pro kontrolu, jestli je zpráva přerušená

// vlákno reagující na přerušení pomocí tlačítka
void pressed() {
    message_abort = true;
}

// vlákno způsobující blikání LED diody 
void flip() {
    myled = !myled;
}

void Tx_interrupt(); // popuje data z bufferu dokud není prázný
void Rx_interrupt(); // přijímá data dokud není buffer úplně prázdný
bool send_buffer(const uint8_t* msg, unsigned int size); // odesílá zapozdřený buffer po bajtu přes počítač 
bool receive_message(uint8_t *msg_buf, int size, int *len); // přijímá buffer přes sériovou linku z počítače
bool send_message(const message *msg, uint8_t *buf, int size); // odesílá data přes sériovou linku do počítače
int computJulianSet(double re, double im, int k, double cre, double cim); // fce která počítá počet iterací na jednoum pixelu a vrací počet proběhlích iterací 

 // velikost bufferu
#define BUF_SIZE 255
 
// yadefinování jednotlivých bufferů 
char tx_buffer[BUF_SIZE];
char rx_buffer[BUF_SIZE];
 
// pointers to the circular buffers
volatile int tx_in = 0;
volatile int tx_out = 0;
volatile int rx_in = 0;
volatile int rx_out = 0;

#define MESSAGE_SIZE (sizeof(message))
#define FLIP_PERIOD 0.3

int main() {
    pc.baud(115200); // nastavení rychlosti komunikace
    pc.attach(&Rx_interrupt, Serial::RxIrq); // attach interrupt handler to receive data
    pc.attach(&Tx_interrupt, Serial::TxIrq); // attach interrupt handler to transmit data
    
    abort_btn.rise(&pressed); // nastavení, aby vlákno reagovalo na přerušení pomocí tlačítka
    
    // výčíst nějaký odpad ye sériové linky
    while (pc.readable()) 
        pc.getc();
        
    // nastavení startaup message při spuštění bufferu
    message msg = { .type = MSG_STARTUP, .data.startup.message = {'P', 'R', 'G', '-', 'L', 'A', 'B', '1', '0'}};
    uint8_t msg_buf[MESSAGE_SIZE];
    int msg_len;
    
    if (fill_message_buf(&msg, msg_buf, MESSAGE_SIZE, &msg_len)) {
        for (int i = 0; i < msg_len; i++) {
            while (! pc.writable());
            pc.putc(msg_buf[i]); 
        }
    }
    
    // Hlavní jednotlivé struktury s zapouzdřenými daty
    struct {
        double c_re;  // re (x) part of the c constant in recursive equation
        double c_im;  // im (y) part of the c constant in recursive equation
        double d_re;  // increment in the x-coords
        double d_im;  // increment in the y-coords
        uint8_t n;    // number of iterations per each pixel
        bool set_data; // constatnt for check if data was set 
    } data = {0,0,0,0,0,false};
    
    struct {
        uint8_t cid; // chunk id
        double re;    // start of the x-coords (real)
        double im;    // start of the y-coords (imaginary)
        uint8_t n_re; // number of cells in x-coords
        uint8_t n_im; // number of cells in y-coords
        uint16_t task_id; // number which is comp in thi row
        bool check_compute; // 
        uint8_t i_re; // pixel právě počítaný na ose x 
        uint8_t i_im; // pixel právé počítanz na ose y
        uint8_t iter; 
        double tmp_re;
        double tmp_im;
    } comp = {0,0,0,0,0,false,0,0,0,0};
    
    
    while(1) { 
        // pokud-li se něco dostane do bufferu, tak se okamžitě začnou zprávy zpracovávat 
        if (rx_in != rx_out) {
            if (receive_message(msg_buf, MESSAGE_SIZE, &msg_len)) { // zpracování zprávy do bufferu
                if (parse_message_buf(msg_buf, msg_len, &msg)) { // zpracování zprávy do struktury msg
                    // jednotlive typy zpráv
                    switch (msg.type) {
                        case MSG_GET_VERSION:
                            msg.type = MSG_VERSION;
                            msg.data.version.major = 0;
                            msg.data.version.minor = 1;
                            msg.data.version.patch = 2;
                            if (!send_message(&msg, msg_buf, msg_len)) // send message pc
                            {
                                msg.type = MSG_ERROR;
                                send_message(&msg, msg_buf, msg_len);                                
                            }                         
                            break;
                        case MSG_ABORT:
                            msg.type = MSG_OK;
                            if (!send_message(&msg, msg_buf, msg_len)) // send message pc
                            {
                                msg.type = MSG_ERROR;
                                send_message(&msg, msg_buf, msg_len);                                
                            }
                            comp.check_compute = false;
                            ticker_led.detach();
                            // resetování dat jako přípravu na další výpočet
                            comp.i_re = 0;
                            comp.task_id = 0;
                            comp.i_im = 0;
                            comp.re = 0;
                            comp.im = 0;
                            comp.tmp_re = 0;
                            comp.tmp_im = 0;
                            myled = 0; // resetování LED diody na stav vypnuto
                            break;
                        case MSG_COMPUTE:
                            if (data.set_data)
                            { 
                                comp.cid = msg.data.compute.cid; // chunk id
                                comp.re = msg.data.compute.re;    // start of the x-coords (real)
                                comp.im = msg.data.compute.im;    // start of the y-coords (imaginary)
                                comp.n_re = msg.data.compute.n_re; // number of cells in x-coords
                                comp.n_im = msg.data.compute.n_im; // number of cells in y-coords
                                ticker_led.attach(&flip, FLIP_PERIOD); // start flashing LED on nukleo
                                comp.check_compute = true; // control variable for start compute
                                message_abort = false;
                                msg.type = MSG_OK; // message for pc => state ok
                            
                                if (!send_message(&msg, msg_buf, msg_len)) // send message pc
                                {
                                    msg.type = MSG_ERROR;
                                    send_message(&msg, msg_buf, msg_len);                                
                                }
                            }  else {
                                msg.type = MSG_ERROR;
                                send_message(&msg, msg_buf, msg_len);
                            }
                            break;
                        case MSG_SET_COMPUTE:
                            data.c_re = msg.data.set_compute.c_re;  // re (x) part of the c constant in recursive equation
                            data.c_im = msg.data.set_compute.c_im;  // im (y) part of the c constant in recursive equation
                            data.d_re = msg.data.set_compute.d_re;  // increment in the x-coords
                            data.d_im = msg.data.set_compute.d_im; // increment in the y-coords
                            data.set_data = true; // constatnt for check if data was set 
                            data.n = msg.data.set_compute.n; // number of iterations per each pixel
                            msg.type = MSG_OK; // message for pc => state ok
                            if (!send_message(&msg, msg_buf, msg_len)) // send message pc
                            {
                                msg.type = MSG_ERROR;
                                send_message(&msg, msg_buf, msg_len);                                
                            }
                            break;
                        default:
                            break;
                    }
                } else {
                    msg.type = MSG_ERROR;
                    send_message(&msg, msg_buf, msg_len);
                }    
            } else {
                msg.type = MSG_ERROR;
                send_message(&msg, msg_buf, msg_len);            
            }
        } else {
            // kontrola jestli se spustil výpočet
            if (comp.check_compute) {
                // kontrolní podmínka na ukončení výpočtu, když je vše spočítáno nebo když je zmáčknuté tlačítko
                if (comp.task_id <  comp.n_im * comp.n_re  && !message_abort) {
                    
                    comp.iter = 0; // počítadlo jednotlivých iterací
                    
                    comp.task_id++; // počítadlo pixelů

                    // Určování souřadnic
                    if (comp.task_id == 1 )
                    {
                        comp.tmp_re = comp.re; // nastavení reálné složky na maximální hodnotu před zahájením výpočtu
                        comp.tmp_im = comp.im; // nastavení maxmální hodnoty komlexního čísla připraveného k odečítání
                        comp.i_re = 0;
                        comp.i_im = 0;
            
                    } else  {                        
                        // přičítání hodnoty k reálné složce a posouvání se tím po reálné ose, když počet pixelů nenpřesáhne
                        // počet pixelů na jednom řádku
                        comp.tmp_re += data.d_re;
                        comp.i_re++;                                       
                    } 
                    
                    // pokud-li se dokončí řádek a přehoupne se na další v chunku
                    if ( (comp.task_id != 1) && ((comp.task_id % comp.n_re) == 1))
                    {
                       // odečítání hodnoty od imaginální složky a posouvání se tím po imaginální ose a reset reálné složky, když počet pixelů přesáhne
                        //  počet pixelů na řádku obrázku
                        comp.tmp_im += data.d_im;
                        comp.tmp_re = comp.re;
                        comp.i_re = 0;
                        comp.i_im++;                                        
                    }
                            
                    // počítání iterací
                    comp.iter = computJulianSet(comp.tmp_re,comp.tmp_im, data.n,data.c_re, data.c_im);

 
                    msg.type = MSG_COMPUTE_DATA; // předání typu posílané zprávy
                    msg.data.compute_data.cid = comp.cid; // označení jednoho řádku 
                    msg.data.compute_data.i_re = comp.i_re; // předání reálné složky počítaného pixelu 
                    msg.data.compute_data.i_im = comp.i_im; // předání imaginární složky počítaného pixelu
                    msg.data.compute_data.iter = comp.iter; // předání počtu proběhlých iterací na pixelu
            
                    send_message(&msg, msg_buf, msg_len);

                } else { // při ukončení výpočtu poslat zprávu pc a vypnutí blikání led diody
                    comp.check_compute= false; // ukončení vzpočtu, aby neběžel na prázno po výpočtu
                    
                    // zaslání zprávy na kvůli na ukončení výpočtu tlačítkem nebo uživatelem 
                    if (message_abort)
                    {
                        msg.type = MSG_ABORT;
                    } else { // poslání zprávy o vypočtení celé zprávy
                        msg.type = MSG_DONE;
                    }
                    ticker_led.detach(); // ukončení blikání LED diody
                    myled = 0; // resetování LED diody na stav vypnuto
                    // resetování dat jako přípravu na další výpočet
                    comp.i_re = 0;
                    comp.task_id = 0;
                    comp.i_im = 0;
                    comp.re = 0;
                    comp.im = 0;
                    comp.tmp_re = 0;
                    comp.tmp_im = 0;
                    send_message(&msg, msg_buf, msg_len);
                }
            } else 
                sleep(); // čekání nuklea na další akce od uživatele
        }
    }
}

void Tx_interrupt()
{
    // send a single byte as the interrupt is triggered on empty out buffer 
    if (tx_in != tx_out) {
        pc.putc(tx_buffer[tx_out]);
        tx_out = (tx_out + 1) % BUF_SIZE;
    } else { // buffer sent out, disable Tx interrupt
        USART2->CR1 &= ~USART_CR1_TXEIE; // disable Tx interrupt
    }
    return;
}

void Rx_interrupt()
{
    // receive bytes and stop if rx_buffer is full
    while ((pc.readable()) && (((rx_in + 1) % BUF_SIZE) != rx_out)) {
        rx_buffer[rx_in] = pc.getc();
        rx_in = (rx_in + 1) % BUF_SIZE;
    }
    return;
}


bool send_buffer(const uint8_t* msg, unsigned int size)
{
    if (!msg && size == 0) {
        return false;    // size must be > 0
    }
    int i = 0;
    NVIC_DisableIRQ(USART2_IRQn); // start critical section for accessing global data
    USART2->CR1 |= USART_CR1_TXEIE; // enable Tx interrupt on empty out buffer
    bool empty = (tx_in == tx_out);
    while ( (i == 0) || i < size ) { //end reading when message has been read
        if ( ((tx_in + 1) % BUF_SIZE) == tx_out) { // needs buffer space
            NVIC_EnableIRQ(USART2_IRQn); // enable interrupts for sending buffer
            while (((tx_in + 1) % BUF_SIZE) == tx_out) {
                /// let interrupt routine empty the buffer
            }
            NVIC_DisableIRQ(USART2_IRQn); // disable interrupts for accessing global buffer
        }
        tx_buffer[tx_in] = msg[i];
        i += 1;
        tx_in = (tx_in + 1) % BUF_SIZE;
    } // send buffer has been put to tx buffer, enable Tx interrupt for sending it out
    USART2->CR1 |= USART_CR1_TXEIE; // enable Tx interrupt
    NVIC_EnableIRQ(USART2_IRQn); // end critical section
    return true;
}

bool receive_message(uint8_t *msg_buf, int size, int *len)
{
    bool ret = false;
    int i = 0;
    *len = 0; // message size
    NVIC_DisableIRQ(USART2_IRQn); // start critical section for accessing global data
    while ( ((i == 0) || (i != *len)) && i < size ) {
        if (rx_in == rx_out) { // wait if buffer is empty
            NVIC_EnableIRQ(USART2_IRQn); // enable interrupts for receing buffer
            while (rx_in == rx_out) { // wait of next character
            }
            NVIC_DisableIRQ(USART2_IRQn); // disable interrupts for accessing global buffer
        }
        uint8_t c = rx_buffer[rx_out];
        if (i == 0) { // message type
            if (get_message_size(c, len)) { // message type recognized
                msg_buf[i++] = c;
                ret = *len <= size; // msg_buffer must be large enough
            } else {
                ret = false;
                break; // unknown message
            }
        } else {
            msg_buf[i++] = c;
        }
        rx_out = (rx_out + 1) % BUF_SIZE;
    }
    NVIC_EnableIRQ(USART2_IRQn); // end critical section
    return ret;
}

bool send_message(const message *msg, uint8_t *buf, int size) {
    return fill_message_buf(msg, buf, MESSAGE_SIZE, &size)
                        && send_buffer(buf, size);

}

int computJulianSet(double re, double im, int k, double cre, double cim)
{

    int iteration = 0; // počet iterací, které vrátí
    while (iteration < k)
    {
        iteration++;
        //pow2complex(&re, &im);
        
        double tmpRe = re;
        re = (re) * (re) + (-1) * (im) * (im);
        im = 2 *(tmpRe) * (im);
        
        //addcomplex(&re, &im, cre, cim);
        re = re + cre;
        im = im + cim;

        // výpočet absolutní hodnoty z komlexního čísla
        double z_abs = sqrt((re) * (re)+(im) * (im));
        //double z_abs = 0; 
        // podmínka na ukončení iterací a vrácení hodnoty z níž se bude počítat barva
        if (z_abs >= 2)
        {
            break;
        }
        
    }
    return iteration;
}