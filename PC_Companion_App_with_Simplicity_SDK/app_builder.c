#include "app_builder.h"
#include "stdint.h"

static uint8_t mirror_byte(uint8_t b) {
    b = ((b >> 1) & 0x55) | ((b << 1) & 0xAA);
    b = ((b >> 2) & 0x33) | ((b << 2) & 0xCC);
    b = (b >> 4) | (b << 4);
    return b;
}

size_t app_builder_bitstringToBytes(const char *input, uint8_t *output) {
    size_t out_index = 0;
    uint8_t current = 0;
    int bit_count = 0;

    for (size_t i = 0; input[i] != '\0'; i++) {
        if (input[i] == '\n' || input[i] == ' ') continue;

        if (input[i] == '1') {
            current = (current << 1) | 1;
            bit_count++;
        } else if (input[i] == '0') {
            current <<= 1;
            bit_count++;
        }

        if (bit_count == 8) {
            output[out_index++] = mirror_byte(current);
            current = 0;
            bit_count = 0;
        }
    }

    if (bit_count > 0) {
        current <<= (8 - bit_count);
        output[out_index++] = mirror_byte(current);
    }

    return out_index;
}
