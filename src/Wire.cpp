#include "Wire.h"

extern "C" {
#include "nrf_delay.h"
#include "app_error.h"
#include "app_util_platform.h"
}

void delay(uint32_t ms){
  nrf_delay_ms(ms);
}

TwoWire::TwoWire(int sclPin, int sdaPin)
  : _sclPin(sclPin), _sdaPin(sdaPin)
{
}

void TwoWire::begin() {
    ret_code_t ret_code;

    _twi_instance = NRF_DRV_TWI_INSTANCE(1);

    nrf_drv_twi_config_t twi_config;
    twi_config.scl = _sclPin;
    twi_config.sda = _sdaPin;
    twi_config.frequency = NRF_TWI_FREQ_400K;
    twi_config.interrupt_priority = APP_IRQ_PRIORITY_LOW;

    ret_code = nrf_drv_twi_init(&_twi_instance, &twi_config, NULL); // Initiate twi driver with instance and configuration values
    APP_ERROR_CHECK(ret_code); // Check for errors in return value

    nrf_drv_twi_enable(&_twi_instance); // Enable the TWI instance
}


void TwoWire::beginTransmission(uint8_t address) {
    _txAddress = address;
    _txBufferLength = 0;
}
void TwoWire::beginTransmission(int address) {
    beginTransmission((uint8_t) address);
}

uint8_t TwoWire::endTransmission() {
    ret_code_t ret_code;

    ret_code = nrf_drv_twi_tx(&_twi_instance, _txAddress, _txBuffer, _txBufferLength, false);
    APP_ERROR_CHECK(ret_code); // Check for errors in return value

    return ret_code;
}

size_t TwoWire::send(uint8_t data) {
    if (_txBufferLength >= BUFFER_LENGTH){
        return 0;
    }

    _txBuffer[_txBufferLength++] = data;
    return 1;
}

int TwoWire::receive(){
    return 0;
}

uint8_t TwoWire::requestFrom(int, int){
    return 0;
}
