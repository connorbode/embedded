#define CPU_PRESCALE(n) (CLKPR = 0x80, CLKPR = (n))
#define CPU_16MHz       0x00
#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include "usb_serial.h"
#include <string.h>


/**

  Question 1
	
	This question is incomplete.  

	Timer1 has been set up properly to set the intensity of the two LEDs.  Pins PB6 and PB7 have
	been used.

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


char SERIAL_READ_TOO_LONG[100] = "Serial data exceeded buffer length of 100 characters.  The data was trimmed.\n";
char SETTING_OUTPUT_COMPARE[100] = "Setting output compare.\n";

char buffer[100]; 					// input buffer for reading from serial
int i = 0;							// counter for input buffer
int val;							// converted value from the buffer
int pin1_value; 					// the value of pin1 (OCR1A)
int pin2_value; 					// the value of pin2 (OCR1B)
int pin1_blink_interval = 1000; 	// the interval at which pin1 blinks (in ms)
int pin2_blink_interval = 2000; 	// the interval at which pin2 blinks (in ms)
int cycle_counter = 0;				// counts the number of clock cycles that have passed (used for blink interval)

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
	TCCR1B |= (1<<WGM12);

	// clear on compare match when up counting,
	// set on compare match when down counting
	TCCR1A |= (0<<COM1A0) | (1<<COM1A1);
	TCCR1A |= (0<<COM1B0) | (1<<COM1B1);

	// set no prescaling
	TCCR1B |= (1<<CS10) | (0<<CS11) | (0<<CS12);





	/**
	 * Set up timer0 for interrupts
	 */

	// prescale timer to clk/1024
	TCCR0A |= (1<<COM0A2) | (0<<COM0A1) | (1<<COM0A0);

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

		// handle serial input
		if (i > 0) {
			buffer[i] = '\0';
			val = atoi(buffer);
			usb_serial_write(SETTING_OUTPUT_COMPARE, strlen(SETTING_OUTPUT_COMPARE));
			usb_serial_write(buffer, strlen(buffer));
			usb_serial_putchar('\n');
			OCR1A = val;
			OCR1B = val;
			i = 0;
		}
	}
}



//
// Interrupt handler
//
ISR(TIMER0_OVF_vect)
{
	// 
}	
