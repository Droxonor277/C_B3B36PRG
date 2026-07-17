#include "mbed.h"

DigitalOut myled(LED1);
Serial serial (SERIAL_TX, SERIAL_RX);

Ticker ticker;

void flip()
{
    myled = !myled;
    if (myled == 0 )
    {
        while (!serial.writable()) {}
        serial.putc('o');
    }
    if (myled == 1)
    {
        while (!serial.writable()) {}
        serial.putc('x');
    }
}

int main() 
{
    serial.baud(115200);
    for(int i = 0; i < 10; i++)
    {
        flip();
        wait(0.5);
    }
    while (serial.readable())
    {
        serial.getc();
    }
    serial.putc('i');
    
    int charakter;
    bool control = true;
    while (control)
    {
        while (!serial.readable()) {}
        char c = serial.getc();
        _Bool ok = true;
        charakter = 0;
        switch (c) {
            case 's':
                ticker.detach();
                myled = 1;
                break;
            case 'e':
                ticker.detach();
                myled = 0;
                break;
            case '5':
                ticker.attach(&flip,1);
                break;
            case '4':
                ticker.attach(&flip,0.5);
                break;
            case '3':
                ticker.attach(&flip,0.2);
                break;
            case '2':
                ticker.attach(&flip,0.1);
                break;
            case '1':
                ticker.attach(&flip,0.05);
                break;
            case 'h':
                while (!serial.writable()) {}
                serial.putc('h');
                charakter = 1;
                break;
            case 'b':
                while (!serial.writable()) {}
                serial.putc('b');
                ticker.detach();
                myled = 0;
                charakter = 1;
                control = false;
                break;
            ok = false;
        } 
        if (charakter != 1)
        {    
            while (!serial.writable()) {}
            serial.putc(ok ? 'a' : 'f');
        } 
    }
    
}