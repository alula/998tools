#pragma once

#include <span>
#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>

struct File998
{
    uint16_t width;
    uint16_t height;
    uint16_t bpp;

    std::vector<uint8_t> data;
};

template <typename T>
struct checked_span : public std::span<T>
{
    // using std::span<T>::span;
    checked_span(std::span<T> const &span) : std::span<T>(span) {}

    inline auto &at(size_t i)
    {
        if (i >= this->size())
            throw std::out_of_range("index out of range");
        return this->operator[](i);
    }

    inline auto const &at(size_t i) const
    {
        if (i >= this->size())
            throw std::out_of_range("index out of range");
        return this->operator[](i);
    }
};

static inline std::string strip_ext(std::string const &input)
{
#ifdef _WIN32
    constexpr auto dir_sep = '\\';
#else
    constexpr auto dir_sep = '/';
#endif

    auto sep_pos = input.find_last_of(dir_sep);
    std::string path;
    std::string filename;

    if (sep_pos != std::string::npos)
    {
        path = input.substr(0, sep_pos + 1);
        filename = input.substr(sep_pos + 1);
    }
    else
    {
        filename = input;
    }

    auto dot_pos = filename.find_last_of('.');
    if (dot_pos != std::string::npos)
    {
        filename.resize(dot_pos);
    }

    return path + filename;
}
