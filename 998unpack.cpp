#include <cstdint>
#include <cstdio>
#include <vector>
#include <fstream>
#include <span>
#include <ranges>
#include <array>

#include "common.h"
#include "compress.h"
#include "stb_image_write.h"

int main(int argc, char *argv[])
{
    printf("998unpack - Microsoft 998 -> PNG image format unpacker - © 2023 Alula\n\n");

    if (argc == 1)
    {
        printf("The output file(s) will be named <input without extension>.png\n\n");

        printf("Usage: %s <file1> <file2> ... <fileN>\n", argv[0]);
        return 0;
    }

    printf("Processing %d files...\n", argc - 1);

    for (int i = 1; i < argc; i++)
    {
        std::string input = argv[i];
        std::string output = strip_ext(input) + ".png";

        printf("Unpacking: %s -> %s\n", input.c_str(), output.c_str());

        std::ifstream file(input, std::ios::binary);

        File998 image{};

        auto read = [&]<typename T>(T &value)
        {
            file.read(reinterpret_cast<char *>(&value), sizeof(T));
        };

        bool compressed = true;

        read(image.width);
        read(image.height);
        read(image.bpp);

        std::array<uint32_t, 256> palette;

        // idfk what this format is and I don't want to bother
        // if (image.bpp == 0)
        // {
        //     uint8_t unknown;
        //     read(unknown);
        //     image.bpp = 8;
        // }
        // else
        if (image.bpp == 1)
        {
            image.bpp = 8;
            uint8_t entry_count;
            read(entry_count);

            for (int i = 0; i < entry_count; i++)
            {
                uint32_t color = 0;
                read(color);

                if ((color & 0xff000000) == 0)
                    color |= 0xff000000;

                uint32_t oc = color;
                color &= 0xff00ff00;
                color |= ((oc >> 16) & 0xff);
                color |= ((oc << 16) & 0xff0000);

                palette[i] = color;
            }
        }
        else if (image.bpp != 24 && image.bpp != 32)
        {
            printf("Error: Invalid color depth, unsupported or not a 998 file?\n");
            return 1;
        }

        auto stride = (image.width * (image.bpp >> 3) + 3) & 0xFFFFFFFC;

        size_t decompressedSize = image.height * stride;
        image.data.resize(decompressedSize, 0);

        std::vector<uint8_t> compressedData;
        auto pos = file.tellg();
        file.seekg(0, std::ios::end);
        auto end = file.tellg();
        file.seekg(pos);

        compressedData.resize(end - pos);
        file.read(reinterpret_cast<char *>(compressedData.data()), compressedData.size());

        if (compressed)
        {
            MsoUncompressLZW(compressedData, image.data);
        }
        else
        {
            image.data = std::move(compressedData);
            image.data.resize(decompressedSize);
        }

        stbi_flip_vertically_on_write(1);

        if (image.bpp == 8)
        {
            std::vector<uint8_t> pixels = std::move(image.data);
            image.bpp = 32;

            stride = (image.width * (image.bpp >> 3) + 3) & 0xFFFFFFFC;
            decompressedSize = image.height * stride;

            image.data.resize(decompressedSize);

            uint32_t *data = reinterpret_cast<uint32_t *>(image.data.data());

            for (uint32_t i = 0; i < static_cast<uint32_t>(image.width) * static_cast<uint32_t>(image.height); i++)
            {
                data[i] = palette[pixels[i]];
            }
        }
        else
        {
            // swap RGB -> BGR
            for (uint32_t y = 0; y < image.height; y++)
            {
                for (uint32_t x = 0; x < image.width; x++)
                {
                    size_t offset = y * stride + x * (image.bpp / 8);
                    std::swap(image.data[offset + 0], image.data[offset + 2]);
                }
            }
        }

        stbi_write_png(output.c_str(), image.width, image.height, image.bpp / 8, image.data.data(), stride);
    }

    return 0;
}