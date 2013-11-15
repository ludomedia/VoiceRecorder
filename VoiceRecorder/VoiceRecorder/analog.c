/*
 * analog.c
 *
 * Created: 13.11.2013 17:48:13
 *  Author: marcha
 */ 

#include <avr/io.h>

void adc_init() {
  ADMUX |= (1 << REFS0) | (1 << ADLAR); // Set ADC reference to AVCC
  ADCSRA |= (1 << ADEN);  // Enable ADC
  ADCSRA |= (1 << ADATE); // Enable auto-triggering
  ADCSRA |= (1 << ADIE);  // Enable ADC Interrupt
  //  sei();                 // Enable Global Interrupts
}  

#define BUFFER_SIZE 128

static uint16_t in_ptr = 0;
static uint16_t bufsize = 0;
static int8_t buffer[BUFFER_SIZE];
static uint16_t maxsize = 0;

volatile long cnt;
const int chipSelect = 10;
#define TEST 9

ISR(ADC_vect)            //ADC interrupt
{
	//uint8_t high,low;
	//low = ADCL;            //Make certain to read ADCL first, it locks the values
	//high = ADCH;            //and ADCH releases them.

	//aval = (high << 8) | low;
	buffer[in_ptr++] = ADCH;

	if(in_ptr >= BUFFER_SIZE) in_ptr = 0;
	if(bufsize < BUFFER_SIZE) bufsize++;
	if(bufsize > maxsize) maxsize = bufsize;
}

uint16_t adc_dataAvailable() {
	uint16_t v;
	cli();
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
/*





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