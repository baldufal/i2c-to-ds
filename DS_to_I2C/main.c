// Inspired by https://gist.github.com/stecman/9ec74de5e8a5c3c6341c791d9c233adc

#ifndef F_CPU
# define F_CPU 8000000UL
#endif

//1Wire
#include "ds18b20.h"
#include "onewire.h"
#include "pindef.h"

//I2C
#include "twislave.h"

//CRC
#include "crc.h"

// AVR
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

// C
#include <string.h>


// I2C slave address.
#define I2C_SLAVE_ADRESS 0x3F

// Status register flags.
#define BIT_INIT_BUS0 0x01
#define BIT_INIT_BUS1 0x02
#define BIT_INIT_BUS2 0x04
#define BIT_INIT_BUS3 0x08
#define BIT_READOUT 0x10



const static gpin_t busPins[4] = {
	{ &PORTD, &PIND, &DDRD, PIND0 },
	{ &PORTD, &PIND, &DDRD, PIND1 },
	{ &PORTD, &PIND, &DDRD, PIND2 },
	{ &PORTD, &PIND, &DDRD, PIND3 }};


	// Contains all known 1wire addresses. 8 Bytes each.
	// First all on bus 1, then bus 3, ...
	static uint8_t deviceAddresses[400];
	// Contains how many devices there are on bus 1 .. 4.
	static uint8_t deviceCounts[4];


	static void io_init(){
		// 1Wire busses: PD0..PD3
		// 7 "GPIO" pins: B0, D7, D6, D5, B7, B6, D4
		// LEDs: C0 (1Hz alive signal), C1 (="1Wire action in progress")
		
		// LEDs as outputs
		DDRC |= 0b00000011;
		// LEDs off
		PORTC &= ~0b00000011;
		
		// Bus and GPIO pins are continuously managed during execution
	}
	

	static void syncGPIO(){
		// 7 "GPIO" pins: B0, D7, D6, D5, B7, B6, D4
		
		// 0 -> B0
		if(i2cdata[1] & (1<<0)){
			DDRB |=  (1<< DDRB0);
			if(i2cdata[2] & (1<<0)){
				PORTB |= (1<<PORTB0);
				}else{
				PORTB &= ~(1<<PORTB0);
			}
			}else{
			DDRB &= ~(1<<DDRB0);
			i2cdata[2] &= ~(1<<0);
			if(PINB & (1<<PINB0))
			i2cdata[2] |= (1<<0);
		}
		// 1 -> D7
		if(i2cdata[1] & (1<<1)){
			DDRD |=  (1<< DDRD7);
			if(i2cdata[2] & (1<<1)){
				PORTD |= (1<<PORTD7);
				}else{
				PORTD &= ~(1<<PORTD7);
			}
			}else{
			DDRD &= ~(1<<DDRD7);
			i2cdata[2] &= ~(1<<1);
			if(PIND & (1<<PIND7))
			i2cdata[2] |= (1<<1);
		}
		// 2 -> D6
		if(i2cdata[1] & (1<<2)){
			DDRD |=  (1<< DDRD6);
			if(i2cdata[2] & (1<<2)){
				PORTD |= (1<<PORTD6);
				}else{
				PORTD &= ~(1<<PORTD6);
			}
			}else{
			DDRD &= ~(1<<DDRD6);
			i2cdata[2] &= ~(1<<2);
			if(PIND & (1<<PIND6))
			i2cdata[2] |= (1<<2);
		}
		// 3 -> D5
		if(i2cdata[1] & (1<<3)){
			DDRD |=  (1<< DDRD5);
			if(i2cdata[2] & (1<<3)){
				PORTD |= (1<<PORTD5);
				}else{
				PORTD &= ~(1<<PORTD5);
			}
			}else{
			DDRD &= ~(1<<DDRD5);
			i2cdata[2] &= ~(1<<3);
			if(PIND & (1<<PIND5))
			i2cdata[2] |= (1<<3);
		}
		// 4 -> B7
		if(i2cdata[1] & (1<<4)){
			DDRB |=  (1<< DDRB7);
			if(i2cdata[2] & (1<<4)){
				PORTB |= (1<<PORTB7);
				}else{
				PORTB &= ~(1<<PORTB7);
			}
			}else{
			DDRB &= ~(1<<DDRB7);
			i2cdata[2] &= ~(1<<4);
			if(PINB & (1<<PINB7))
			i2cdata[2] |= (1<<4);
		}
		// 5 -> B6
		if(i2cdata[1] & (1<<5)){
			DDRB |=  (1<< DDRB6);
			if(i2cdata[2] & (1<<5)){
				PORTB |= (1<<PORTB6);
				}else{
				PORTB &= ~(1<<PORTB6);
			}
			}else{
			DDRB &= ~(1<<DDRB6);
			i2cdata[2] &= ~(1<<5);
			if(PINB & (1<<PINB6))
			i2cdata[2] |= (1<<5);
		}
		// 6 -> D4
		if(i2cdata[1] & (1<<6)){
			DDRD |=  (1<< DDRD4);
			if(i2cdata[2] & (1<<6)){
				PORTD |= (1<<PORTD4);
				}else{
				PORTD &= ~(1<<PORTD4);
			}
			}else{
			DDRD &= ~(1<<DDRD4);
			i2cdata[2] &= ~(1<<6);
			if(PIND & (1<<PIND4))
			i2cdata[2] |= (1<<6);
		}
	}

	// Search all devices of a specific bus.
	// Result is written into deviceAddresses[] and i2cdata[].
	static void _searchDevices(uint8_t busIndex){
		memset(i2cdata + 3, 0, sizeof(i2cdata) -3);
		
		// Start index of this bus in deviceAddresses[].
		uint8_t addressStartIndex = 0;
		for(uint8_t i = 0; i < busIndex; i++){
			addressStartIndex += 8* deviceCounts[i];
		}
		onewire_reset(busPins + busIndex);
		
		// Prepare a new device search
		onewire_search_state search;
		onewire_search_init(&search);
		
		// Search and dump temperatures until we stop finding devices
		uint8_t foundDevices = 0;
		while (onewire_search(busPins + busIndex, &search)) {
			if (onewire_check_rom_crc(&search)) {
				memcpy(deviceAddresses + addressStartIndex + 8 * foundDevices, search.address, sizeof(search.address));
			}
			foundDevices++;
		}
		deviceCounts[busIndex] = foundDevices;
		
		// Write data to i2c registers.
		uint8_t length = deviceCounts[busIndex] * 8;
		i2cdata[4] = length;
		memcpy(i2cdata + 5, deviceAddresses + addressStartIndex, length);
		i2cdata[3] = crc8(i2cdata + 4, length + 1);
	}

	// Read all devices of a specific bus.
	// Result is written into i2cdata[].
	static void _readDevices(uint8_t busIndex){
		// Start index of this bus in deviceAddresses[].
		uint8_t addressStartIndex = 0;
		// Start index of this bus in i2cdata[].
		uint8_t i2cStartIndex = 5;
		for(uint8_t i = 0; i < busIndex; i++){
			addressStartIndex += 8 * deviceCounts[i];
			i2cStartIndex += 2 * deviceCounts[i];
		}
		
		for(uint8_t i = 0; i < deviceCounts[busIndex]; i++){
			uint16_t reading = ds18b20_read_slave(busPins + busIndex, deviceAddresses + addressStartIndex + 8 * i);
			//uint8_t i2cIndex = i2cStartIndex + 2 * i;
			//reading = (i2cIndex << 8) | busIndex; // For debugging only.
			memcpy(i2cdata + i2cStartIndex + 2 * i, &reading, sizeof(reading));
		}
	}


	static void handleStatusByte(){
		// Return if nothing to do
		if((i2cdata[0] & 0b00011111) == 0x00)
		return;
		
		// Enable readout LED
		PORTC |= 0x02;
		
		switch(i2cdata[0]){
			case BIT_INIT_BUS0:
			// When initializing bus 0 we reset all known addresses.
			memset(deviceAddresses, 0, sizeof(deviceAddresses));
			memset(deviceCounts, 0, sizeof(deviceCounts));
			
			_searchDevices(0);
			break;
			
			case BIT_INIT_BUS1:
			_searchDevices(1);
			break;
			
			case BIT_INIT_BUS2:
			_searchDevices(2);
			break;
			
			case BIT_INIT_BUS3:
			_searchDevices(3);
			break;
			
			case BIT_READOUT:
			{
				// Start conversion
				ds18b20_convert(busPins + 0);
				ds18b20_convert(busPins + 1);
				ds18b20_convert(busPins + 2);
				ds18b20_convert(busPins + 3);
				
				/*
				// Start conversion for every device independently.
				uint8_t addressStartIndex = 0;
				for(uint8_t busIndex = 0; busIndex < 4; busIndex++){
				for(uint8_t deviceIndex = addressStartIndex; deviceIndex < addressStartIndex + deviceCounts[busIndex]; deviceIndex++){
				ds18b20_convert_single(busPins + busIndex,deviceAddresses + 8 * deviceIndex);
				}
				addressStartIndex += deviceCounts[busIndex];
				}
				*/
				
				// Wait for conversion to complete.
				_delay_ms(750);
				
				// Clear result.
				memset(i2cdata + 3, 0, sizeof(i2cdata) - 3);
				
				// Get results.
				_readDevices(0);
				_readDevices(1);
				_readDevices(2);
				_readDevices(3);
				
				// Set length and crc.
				i2cdata[4] = 2 * (deviceCounts[0] + deviceCounts[1] + deviceCounts[2] + deviceCounts[3]);
				i2cdata[3] = crc8(i2cdata + 4, i2cdata[4] + 1);
				break;
			}
		}
		
		// Clear action bits
		i2cdata[0x00] &= 0b11100000;
		// Disable readout LED
		PORTC &= ~0x02;
	}


	int main(void)
	{
		memset(i2cdata, 0, sizeof(i2cdata));
		memset(deviceAddresses, 0, sizeof(deviceAddresses));
		memset(deviceCounts, 0, sizeof(deviceCounts));
		
		io_init();
		
		// Enable I2C.
		init_twi_slave(I2C_SLAVE_ADRESS);
		
		// Enable watchdog to restart if we didn't reset it for 2 seconds.
		wdt_enable(WDTO_2S);
		
		while(1){
			// Disable interrupts.
			cli();
			
			syncGPIO();
			handleStatusByte();
			
			// Enable interrupts.
			sei();
			
			_delay_ms(500);
			
			// Toggle health LED
			PORTC ^= (1<<PORTC0);
			
			// Reset watchdog timer.
			wdt_reset();
		}

		return 0;
	}
