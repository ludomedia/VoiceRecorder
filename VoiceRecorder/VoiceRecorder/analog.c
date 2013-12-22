/*
 * analog.c
 *
 * Created: 13.11.2013 17:48:13
 *  Author: marcha
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "analog.h"

void adc_init() {
  ADMUX |= (1 << REFS0) | (1 << ADLAR); // Set ADC reference to AVCC and result left adjusted
  ADCSRA  = (1<< ADPS2) | (1<<ADPS1) | (1<<ADPS0);	// 128 x prescalar
  ADCSRA |= (1 << ADEN);							// Enable ADC
  ADCSRA |= (1 << ADATE);							// Enable auto-triggering
  ADCSRA |= (1 << ADIE);							// Enable ADC Interrupt
  ADCSRA |= (1 << ADSC);							// Start First conversion
}  

#define BUFFER_SIZE 400
/*
static uint16_t in_ptr = 0;
static uint16_t bufsize = 0;
static int8_t buffer[BUFFER_SIZE];
static uint16_t maxsize = 0;
*/
/*volatile long cnt;
const int chipSelect = 10;
#define TEST 9
*/
ISR(ADC_vect)            //ADC interrupt
{
	//uint8_t high,low;
	//low = ADCL;            //Make certain to read ADCL first, it locks the values
	//high = ADCH;            //and ADCH releases them.

	//aval = (high << 8) | low;
	//buffer[in_ptr++] = ADCH;
	PORTD |= (1<<2);

	uint8_t v = ADCH;
	adc_buffer_write(v);
	PORTD &= ~(1<<2);
	//if(in_ptr >= BUFFER_SIZE) in_ptr = 0;
	//if(bufsize < BUFFER_SIZE) bufsize++;
	//if(bufsize > maxsize) maxsize = bufsize;
}

/*
uint16_t adc_dataAvailable() {
	uint16_t v;
	cli(); // TODO look at util/atomic.h
	v = bufsize;
	sei();
	return v;
}

uint8_t adc_getAnalogData() {
	int8_t v = 0;
	cli();
	if(bufsize!=0) {
		if(in_ptr>=bufsize) v = buffer[in_ptr-bufsize];
		else v = buffer[BUFFER_SIZE + in_ptr - bufsize];
		// v = buffer[(BUFFER_SIZE + in_ptr - bufsize) % BUFFER_SIZE];
		bufsize--;
	}
	sei();
	return v;
}
*/
static uint16_t in_ptr = 0;
static uint16_t out_ptr = 0;
volatile uint16_t buffer_size = 0;
static int8_t buffer[BUFFER_SIZE];
static uint8_t buffer_overrun = 0;

void adc_buffer_write(uint8_t v) {
	buffer[in_ptr++] = v;
	if(buffer_size<BUFFER_SIZE) buffer_size++;
	else {
		if(buffer_overrun==0) buffer_overrun = 1;
		if(buffer_overrun==1) {
			puts("Bufovr");
			buffer_overrun = 2;
		}			
		out_ptr++;
		if(out_ptr>=BUFFER_SIZE) out_ptr = 0;
	}		
	if(in_ptr>=BUFFER_SIZE) in_ptr = 0;
}

uint8_t adc_buffer_read() {
	while(buffer_size==0);
	cli();
	uint8_t v = buffer[out_ptr++];
	if(out_ptr>=BUFFER_SIZE) out_ptr = 0;
	buffer_size--;
	sei();
	return v;
}
/*
void adc_dump_buffer() {
	printf("Dumping buffer in: %02x out: %02x\n", in_ptr, out_ptr);
	for(int o=0;o<BUFFER_SIZE;o++) {
		printf("%02x ", buffer[o]);
	}
	printf("\n");
}
*/


/*
  SD card datalogger
 
 This example shows how to log data from three analog sensors 
 to an SD card using the SD library.
 	
 The circuit:
 * analog sensors on analog ins 0, 1, and 2
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 4
 
 created  24 Nov 2010
 modified 9 Apr 2012
 by Tom Igoe
 
 This example code is in the public domain.
 
 Temps de flush = 2ms soit 	 

#include <SD.h>
 */

//#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
//#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.

/*


void setup()
{
 // Open serial communications and wait for port to open:
  Serial.begin(9600);
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  pinMode(TEST, OUTPUT);
  digitalWrite(TEST, LOW);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

  // INIT ADC
  cli();
  TIMSK0 = 0;
  TIMSK1 = 0;
  TIMSK2 = 0;

  cnt = 0;  
  ADMUX |= (1 << REFS0) | (1 << ADLAR); // Set ADC reference to AVCC
  ADCSRA |= (1 << ADEN);  // Enable ADC
  ADCSRA |= (1 << ADATE); // Enable auto-triggering
  ADCSRA |= (1 << ADIE);  // Enable ADC Interrupt
  sei();                 // Enable Global Interrupts
}

void loop()
{
  // make a string for assembling the data to log:
  uint16_t v;
  uint8_t b;
  long maxdiff = 0;
  SD.remove("test.wav");
  File dataFile = SD.open("test.wav", FILE_WRITE);
  if (dataFile) {
    
    while(Serial.available()==0);
    Serial.println("start");

    ADCSRA |= (1 << ADSC);  // Start A2D Conversions
        
    // read three sensors and append to the string:
    for (long count=0;count<((long)500*(long)8192);count++) {
      do {
        pinMode(TEST, OUTPUT);
      } while(dataAvailable()==0);
        v = getAnalogData();     
        //long stamp = millis();
        dataFile.write(v);
        //long diff = millis() - stamp;
        //if(diff > maxdiff) maxdiff = diff;
        //delayMicroseconds(50);
    }
    dataFile.close();
    
    // print to the serial port too:
    Serial.println("done");
    Serial.print("maxsize = ");
    Serial.println(maxsize);
    Serial.print("maxdiff = ");
    Serial.println(maxdiff);
  }  
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  } 
  for(;;);
}

*/