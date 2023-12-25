#include "ds18b20.h"
#include "onewire.h"
#include "pindef.h"

// AVR
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// C
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


/**
 * Set up the ATmega328P's UART peripheral to transmit
 *
 * The contents of this may need to be changed if you're using a different chip
 * or a different CPU frequency.
 */
void init_usart(void)
{
    cli();

    // Set baud rate to 9600 (with 16MHz clock)
	// (See "Examples of Baud Rate Setting" in the ATmega328P datasheet)
    UBRR0H = (uint8_t)(103 >> 8);
    UBRR0L = (uint8_t)(103);

    // Enable receiver and transmitter
    UCSR0B = _BV(RXEN0) | _BV(TXEN0);

    // Set frame format: 8 data, 1 stop bit
    UCSR0C = (3<<UCSZ00);
    UCSR0C &= ~(1<<USBS0);

    sei();
}

/**
 * Send a single byte out over serial (blocking)
 */
void usart_transmit(uint8_t data)
{
    // Wait for empty transmit buffer
    while ( !( UCSR0A & (1<<UDRE0)) );

    // Put data into buffer, sends the data
    UDR0 = data;
}

/**
 * Block until a single byte is received over serial
 */
uint8_t usart_receive(void)
{
    // Wait for data to be received
    while ( !(UCSR0A & (1<<RXC0)) );

    // Get and return received data from buffer
    return UDR0;
}

/**
 * Print a null-terminated string
 */
void print(char* string)
{
    uint8_t i = 0;
    while (string[i] != '\0') {
        usart_transmit(string[i]);
        ++i;
    }
}

/**
 * Print a null-terminated string followed by an automatic line break
 */
void println(char* string)
{
    print(string);
    usart_transmit('\n');
}

/**
 * Convert a fixed-point 12.4 number into a floating point string
 * This is creates a human-readable string for temperature values from a DS18B20
 *
 * The passed buffer must be able to accomodate up to 9 characters plus a null terminator.
 */
void fptoa(uint16_t value, char* buf, uint8_t bufsize)
{
    const uint16_t integer = (value >> 4);
    const uint16_t decimal = (value & 0x0F) * 625; // One 16th is 0.625

    memset(buf, '\0', bufsize);

    // Convert the integer portion of the number to a string
    itoa(integer, buf, 10);

    // Add decimal point
    buf += strlen((char*)buf);
    (*buf++) = '.';

    if (decimal == 0) {
        // Fill zeros to maintain fixed decimal places
        memset(buf, '0', 4);
        return;
    }

    if (decimal < 1000) {
        // Add a leading zero for .0625
        (*buf++) = '0';
    }

    // Convert the decimal portion of the number to a string
    itoa(decimal, buf, 10);
}

const static gpin_t sensorPin = { &PORTD, &PIND, &DDRD, PD3 };

int main(void)
{
    init_usart();

    for (;;) {
        if (!onewire_reset(&sensorPin)) {
            println("Nothing on the bus?");
            _delay_ms(1000);
            continue;
        }

        // Intiate conversion and wait for it to finish
        ds18b20_convert(&sensorPin);
        _delay_ms(750);

        // Prepare a new device search
        onewire_search_state search;
        onewire_search_init(&search);

        // Search and dump temperatures until we stop finding devices
        while (onewire_search(&sensorPin, &search)) {
            if (!onewire_check_rom_crc(&search)) {
                print("Bad ROM CRC");
                continue;
            }

            uint16_t reading = ds18b20_read_slave(&sensorPin, search.address);

            if (reading != kDS18B20_CrcCheckFailed) {
                // You can get the temperature as floating point degrees if needed, though
                // working with floats on small AVRs eats up a lot of cycles and code space:
                //
                //   float temperature = ((float) reading) / 16;
                //

                // Convert fixed-point to a printable string
                char buf[10];
                fptoa(reading, buf, sizeof(buf));
                print(buf);

            } else {
                println("Bad temp CRC");
            }

            print("  ");
        }

        print("\n");
    }

    return 0;
}
