#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))
#define CPU_16MHz       0x00
#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usb_serial.h"
#include <string.h>
#include <stdio.h>


/**

  Question 1

	Timer1 has been set up properly to set the intensity of the two LEDs.  Pins PB6 (pin 1) and 
	PB7 (pin 2) have been used.



	I was just in the process of setting up Timer0 to control interrupts.  My idea is as follows:

		(1) Clock is running at 16MHz

		(2) Timer0 is running at (clk / 1024)

		(3) Figure out how often the Timer0 interrupt is fired (in seconds)

		(4) Record how many seconds have elapsed based on how many interrupts have been fired

		(5) Blink the pin based on the number of interrupts that have been fired & based on the
			blink interval. 

	Serial communication has been set up to set the intensity.  However, both pins are set to the
	same intensity and nothing is done to set the blink interval.  This will have to be modified
	slightly.

**/


char SERIAL_READ_TOO_LONG[100] 	 = "Serial data exceeded buffer length of 100 characters.  The data was trimmed.\n";
char SETTING_OUTPUT_COMPARE[100] = "Setting output compare.\n";
char READ_FROM_ANALOG[100] 		 = "Read the following value from analog in: ";
char NEWLINE 				     = '\n';
char INPUT_NOT_UNDERSTOOD[200] 	 = "The input you provided was not understood.\n Please enter [pin]_[action]_[value], where:\n - pin is 1 or 2 or 3\n - action is F or I\n - value is an appropriate frequency or intensity value\n\n";

char buffer[100]; 					// input buffer for reading from serial
int i = 0;							// counter for input buffer
int val;							// converted value from the buffer
int pin; 							// the pin being modified

int pin1_value = 100;				// the value of pin1 (OCR1A)
int pin2_value = 100;				// the value of pin2 (OCR1B)
int pin1_on = 0;					// whether pin1 is on
int pin2_on = 0;					// whether pin2 is on
int pin1_blink_interval = 1000; 	// the interval at which pin1 blinks (in ms)
int pin2_blink_interval = 1000; 	// the interval at which pin2 blinks (in ms)

int pin1_cycle_counter = 0;			// counts the number of clock cycles that have passed (used for blink interval)
int pin2_cycle_counter = 0;			// counts the number of clock cycles that have passed (used for blink interval)
int read_cycle_counter = 0; 		// counts the number of clock ...

int read_interval = 1000;




//
// Input was not understood..
//
void badInput () {
	usb_serial_write(INPUT_NOT_UNDERSTOOD, strlen(INPUT_NOT_UNDERSTOOD));
}


//
// ADC READ
//
uint16_t adc_read(uint8_t ch)
{
	
	//Set internal reference
    ADMUX=(1<<REFS0);
	
	//Enable, set prescale 
    ADCSRA=(1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); 

	//Input channel
    ch = ch & 0b00000111;
    ADMUX|=ch;

	//Single conversion
    ADCSRA|=(1<<ADSC);

	//Wait for conversion to complete
    while(!(ADCSRA & (1<<ADIF)));

	//Clear ADIF by writing one to it
    ADCSRA|=(1<<ADIF);
 
 	//return 16 bit value
    return(ADC);
}


int main(void)
{

	// set clock speed 
	CPU_PRESCALE(CPU_16MHz);

	// initialize serial communication
	usb_init();



	/**
	 * Analog read
	 */

	// set pin to input mode
	DDRF |= (0<<PF0);



	/**
	 * Set up timer1 & PWM for LEDs
	 */

	// set pins to output mode
	DDRB |= (1<<PB6) | (1<<PB5);

	// set waveform generation to PWM with phase correct
	TCCR1A |= (0<<WGM11) | (1<<WGM10);
	TCCR1B |= (0<<WGM12);

	// clear on compare match when up counting,
	// set on compare match when down counting
	TCCR1A |= (0<<COM1A0) | (1<<COM1A1);
	TCCR1A |= (0<<COM1B0) | (1<<COM1B1);

	// set no prescaling
	TCCR1B |= (1<<CS10) | (0<<CS11) | (0<<CS12);





	/**
	 * Set up timer0 for interrupts
	 */

	// prescale timer to clk / 8 
	TCCR0B |= (0<<CS00) | (1<<CS01) | (0<<CS02);

	// enable interrupts for timer0
	TIMSK0 |= (1 << TOIE0);

	// enable interrupts globally
	sei();



	// loop infinitely
	while (1) {

		// read serial input
		while (usb_serial_available() > 0) {
			buffer[i] = usb_serial_getchar();
			i += 1;

			if (i > 99) {
				usb_serial_write(SERIAL_READ_TOO_LONG, strlen(SERIAL_READ_TOO_LONG));
				break;
			}
		}

		// prevent array index out of bounds..
		if (i > 0 && i < 4) {
			badInput();
			i = 0;
		}


		else if (i > 0) {
			buffer[i] = '\0';
			val = atoi(&buffer[4]);

			if (buffer[0] == '1' && buffer[2] == 'I') {
				pin1_value = val;
			}

			else if (buffer[0] == '2' && buffer[2] == 'I') {
				pin2_value = val;
			}

			else if (buffer[0] == '1' && buffer[2] == 'F') {
				pin1_blink_interval = val;
				pin1_cycle_counter = 0;
			}

			else if (buffer[0] == '2' && buffer[2] == 'F') {
				pin2_blink_interval = val;
				pin2_cycle_counter = 0;
			}

			else if (buffer[0] == '3' && buffer[2] == 'F') {
				read_interval = val;
				read_cycle_counter = 0;
			}

			else {
				badInput();
			}

			i = 0;
		}
	}
}


//
// Interrupt handler
//
ISR(TIMER0_OVF_vect)
{
	// this interrupt fires once every 7ms
	int pin1_ms = pin1_cycle_counter / 7;
	int pin2_ms = pin2_cycle_counter / 7;
	int read_ms = read_cycle_counter / 7;

	// check if we've just crossed the threshold
	if (pin1_ms > pin1_blink_interval) {
		if (pin1_on) {
			OCR1A = 0;
			pin1_on = 0;
		}
		
		else {
			OCR1A = pin1_value;
			pin1_on = 1;
		}

		pin1_cycle_counter = 0;
	}

	// check if we've crossed the threshold
	if (pin2_ms > pin2_blink_interval) {
		if (pin2_on) {
			OCR1B = 0;
			pin2_on = 0;
		}

		else {
			OCR1B = pin2_value;
			pin2_on = 1;
		}

		pin2_cycle_counter = 0;
	}

	// check if we've crossed the threshold
	if (read_ms > read_interval) {
		uint16_t read_int = adc_read(PF0);
		char read_value[6] = {0};
		sprintf(read_value, "%d", read_int);

		usb_serial_write(READ_FROM_ANALOG, strlen(READ_FROM_ANALOG));
		usb_serial_write(read_value, strlen(read_value));
		usb_serial_putchar(NEWLINE);

		read_cycle_counter = 0;
	}

	// increment both cycle counters
	pin1_cycle_counter += 1;
	pin2_cycle_counter += 1;
	read_cycle_counter += 1;
}	
