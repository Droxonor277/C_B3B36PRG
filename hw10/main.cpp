#include "mbed.h"
#include "messages.h"

Serial pc(SERIAL_TX, SERIAL_RX);
DigitalOut myled(LED1);
InterruptIn abort_btn(USER_BUTTON);

// Používám program zpracovaný na cvičení

Ticker ticker_led;

bool message_abort = false;
void pressed() {
    message_abort = true;
}

void flip() {
    myled = !myled;
}

void Tx_interrupt();
void Rx_interrupt();
bool send_buffer(const uint8_t* msg, unsigned int size);
bool receive_message(uint8_t *msg_buf, int size, int *len);
bool send_message(const message *msg, uint8_t *buf, int size);
 
#define BUF_SIZE 255
 
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
    pc.baud(115200);
    pc.attach(&Rx_interrupt, Serial::RxIrq); // attach interrupt handler to receive data
    pc.attach(&Tx_interrupt, Serial::TxIrq); // attach interrupt handler to transmit data
    
    abort_btn.rise(&pressed);
    
    while (pc.readable()) 
        pc.getc();
        
    message msg = { .type = MSG_STARTUP, .data.startup.message = {'P', 'R', 'G', '-', 'L', 'A', 'B', '1', '0'}};
    uint8_t msg_buf[MESSAGE_SIZE];
    int msg_len;
    
    if (fill_message_buf(&msg, msg_buf, MESSAGE_SIZE, &msg_len)) {
        for (int i = 0; i < msg_len; i++) {
            while (! pc.writable());
            pc.putc(msg_buf[i]); 
        }
    }
    
    struct {
        uint16_t chunk_id;
        uint16_t nbr_tasks;
        uint16_t task_id;
        bool computing;
    } computation = { 0, 0, 0, false };
    
    while(1) { 
        if (rx_in != rx_out) {
            if (receive_message(msg_buf, MESSAGE_SIZE, &msg_len)) {
                if (parse_message_buf(msg_buf, msg_len, &msg)) {
                    switch (msg.type) {
                        case MSG_GET_VERSION:
                            msg.type = MSG_VERSION;
                            msg.data.version.major = 0;
                            msg.data.version.minor = 1;
                            msg.data.version.patch = 2;
                            send_message(&msg, msg_buf, msg_len);                            
                            break;
                        case MSG_ABORT:
                            msg.type = MSG_OK;
                            send_message(&msg, msg_buf, msg_len);
                            computation.computing = false;
                            ticker_led.detach();
                            myled = 0;
                            break;
                        case MSG_COMPUTE:
                            computation.chunk_id = msg.data.compute.chunk_id;
                            computation.nbr_tasks = msg.data.compute.nbr_tasks;
                            computation.task_id = 0;
                            computation.computing = true;
                            ticker_led.attach(&flip, FLIP_PERIOD);
                            message_abort = false;
                            msg.type = MSG_OK;
                            send_message(&msg, msg_buf, msg_len);
                        default:
                            break;
                    }
                } else {
                    msg.type = MSG_ERROR;
                    send_message(&msg, msg_buf, msg_len);
                }    
            }
        } else {
            if (computation.computing) {
                if (computation.task_id < computation.nbr_tasks && !message_abort) {
                    wait(0.5);
                    computation.task_id++;
                    msg.type = MSG_COMPUTE_DATA;
                    msg.data.compute_data.chunk_id = computation.chunk_id;
                    msg.data.compute_data.task_id = computation.task_id;
                    msg.data.compute_data.result = rand() % 255;
                    send_message(&msg, msg_buf, msg_len);
                } else {
                    computation.computing = false;
                    if (message_abort)
                        msg.type = MSG_ABORT;
                    else
                        msg.type = MSG_DONE;
                    send_message(&msg, msg_buf, msg_len);
                    ticker_led.detach();
                    myled = 0;
                }
            } else 
                sleep();
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