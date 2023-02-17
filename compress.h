#pragma once

#include "common.h"

#include <span>
#include <vector>
#include <cstdint>

struct MSOLZWDICT
{
    uint8_t suffix;
    uint16_t prefix;
    uint16_t prefixLength;
};

std::vector<uint8_t> MsoCompressLZW(std::span<uint8_t> const &input);
void MsoUncompressLZW(std::span<uint8_t> const &input, std::span<uint8_t> const &output);