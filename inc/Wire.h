#pragma once

#include <stddef.h>
#include <stdint.h>

extern "C" {
#include "nrf_drv_twi.h"
}


void delay(uint32_t);

#define BUFFER_LENGTH (32)

class TwoWire {
public:
    TwoWire(int sclPin, int sdaPin);

    void begin();
    void beginTransmission(uint8_t);
    void beginTransmission(int);
    uint8_t endTransmission(void);
    uint8_t requestFrom(uint8_t, uint8_t);
    uint8_t requestFrom(uint8_t, uint8_t, uint8_t);
    uint8_t requestFrom(uint8_t, uint8_t, uint32_t, uint8_t, uint8_t);
    uint8_t requestFrom(int, int);
    uint8_t requestFrom(int, int, int);

    size_t send(uint8_t);

    int read();
    int receive();

private:
    int _sclPin, _sdaPin;
    nrf_drv_twi_t _twi_instance;

    // RX Buffer
    uint8_t _rxBuffer[BUFFER_LENGTH];
    uint8_t _rxBufferIndex;
    uint8_t _rxBufferLength;

    // TX Buffer
    uint8_t _txAddress;
    uint8_t _txBuffer[BUFFER_LENGTH];
    uint8_t _txBufferLength;
};
