/*
 * analog.h
 *
 * Created: 13.11.2013 17:49:02
 *  Author: marcha
 */ 


#ifndef ANALOG_H_
#define ANALOG_H_

void adc_init();
void adc_buffer_write(uint8_t);
uint8_t adc_buffer_read();
void adc_dump_buffer();

#endif /* ANALOG_H_ */