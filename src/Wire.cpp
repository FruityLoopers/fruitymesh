#include "Wire.h"

//#include "nrf_delay.h"

void delay(int ms){
 // nrf_delay_ms(ms);
}

void TwoWire::beginTransmission(int) {
}

uint8_t TwoWire::endTransmission() {
}

void TwoWire::begin() {
}

size_t TwoWire::send(uint8_t) {
}

int TwoWire::receive(){
	return 0;
}

uint8_t TwoWire::requestFrom(int, int){
	return 0;
}
