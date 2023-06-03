#include "picture.h"

// Later: Endianness
void u32ToBytes(uint32_t val, char *p_buffer)
{
    *p_buffer = static_cast<uint8_t>(val >> 24);
    p_buffer++;
    *p_buffer = static_cast<uint8_t>(val >> 16);
    p_buffer++;
    *p_buffer = static_cast<uint8_t>(val >> 8);
    p_buffer++;
    *p_buffer = static_cast<uint8_t>(val);
    p_buffer++;
}
uint32_t bytesToU32(char *p_buffer)
{
    uint8_t byte_4 = static_cast<uint8_t>(*p_buffer);
    p_buffer++;
    uint8_t byte_3 = static_cast<uint8_t>(*p_buffer);
    p_buffer++;
    uint8_t byte_2 = static_cast<uint8_t>(*p_buffer);
    p_buffer++;
    uint8_t byte_1 = static_cast<uint8_t>(*p_buffer);
    p_buffer++;

    return ((uint32_t)byte_4 << 24) | ((uint32_t)byte_3 << 16) | ((uint32_t)byte_2 << 8) | ((uint32_t)byte_1);
}
