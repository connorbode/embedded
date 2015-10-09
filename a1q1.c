#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))
#define CPU_16MHz       0x00
#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "usb_serial.h"
#include <string.h>


/**

  Question 1

	Timer1 has been set up properly to set the intensity of the two LEDs.  Pins PB6 (pin 1) and 
	PB7 (pin 2) have been used.

	Everything just needs to be documented.. 

**/


char SERIAL_READ_TOO_LONG[100] = "Serial data exceeded buffer length of 100 characters.  The data was trimmed.\n";
char SETTING_OUTPUT_COMPARE[100] = "Setting output compare.\n";
char INPUT_NOT_UNDERSTOOD[200] = "The input you provided was not understood.\n Please enter [pin]_[action]_[value], where:\n - pin is 1 or 2\n - action is F or I\n - value is an appropriate frequency or intensity value\n\n";

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




//
// Input was not understood..
//
void badInput () {
	usb_serial_write(INPUT_NOT_UNDERSTOOD, strlen(INPUT_NOT_UNDERSTOOD));
}


int main(void)
{

	// set clock speed 
	CPU_PRESCALE(CPU_16MHz);

	// initialize serial communication
	usb_init();



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

	// increment both cycle counters
	pin1_cycle_counter += 1;
	pin2_cycle_counter += 1;
}	
