#pragma once

#include <stddef.h>
#include <stdint.h>

void delay(int);

class TwoWire {
public:
    void begin();
    void begin(uint8_t);
    void begin(int);
    void beginTransmission(uint8_t);
    void beginTransmission(int);
    uint8_t endTransmission(void);
    uint8_t endTransmission(uint8_t);
    uint8_t requestFrom(uint8_t, uint8_t);
    uint8_t requestFrom(uint8_t, uint8_t, uint8_t);
    uint8_t requestFrom(uint8_t, uint8_t, uint32_t, uint8_t, uint8_t);
    uint8_t requestFrom(int, int);
    uint8_t requestFrom(int, int, int);
    size_t write(uint8_t);
    size_t write(const uint8_t *, size_t);
    size_t send(uint8_t);
    size_t send(const uint8_t *, size_t);

    int read();
    int receive();
};
