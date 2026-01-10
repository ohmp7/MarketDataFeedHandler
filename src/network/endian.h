#pragma once

#include "event.h"

#include <concepts>
#include <cstdint>

/*
Handling Network-Byte-Order Integers.
*/
template <std::unsigned_integral T>
inline T ReadBigEndian(const std::uint8_t* buf,  Bytes offset) {
    T converted = 0;
    for (Bytes i = 0; i < sizeof(T); ++i) {
        converted <<= 8;
        std::uint8_t next_byte = buf[offset + i];
        converted = converted | static_cast<T>(next_byte);
    }
    return converted;
}

template <std::unsigned_integral T>
inline void WriteBigEndian(std::uint8_t* buf, Bytes offset, T value) {
    for (Bytes i = 0; i < sizeof(T); ++i) {
        buf[offset + sizeof(T) - i - 1] = static_cast<uint8_t>(value & 0xFF);
        if constexpr (sizeof(T) > 1) value >>= 8;
    }
}