#include <cstdint>
#include <cstdio>
#include <vector>
#include <fstream>
#include <span>

#include "common.h"
#include "compress.h"
#include "stb_image.h"

int main(int argc, char *argv[])
{
    printf("998pack - PNG -> Microsoft 998 image format packer - © 2023 Alula\n\n");

    if (argc == 1)
    {
        printf("The output file(s) will be named <input without extension>.998\n\n");

        printf("Usage: %s <file1> <file2> ... <fileN>\n", argv[0]);
        return 0;
    }

    printf("Processing %d files...\n", argc - 1);

    for (int i = 1; i < argc; i++)
    {
        std::string input = argv[i];
        std::string output = strip_ext(input) + ".998";

        printf("Packing: %s -> %s\n", input.c_str(), output.c_str());

        stbi_set_flip_vertically_on_load(true);
        int width, height, bpp;
        uint8_t *data = stbi_load(input.c_str(), &width, &height, &bpp, 4);

        if (!data)
        {
            printf("Error: Failed to load image '%s': %s\n", input.c_str(), stbi_failure_reason());
            return 1;
        }

        if (width > 0xffff || height > 0xffff)
        {
            printf("Error: Image '%s' is too large: %dx%d\n", input.c_str(), width, height);
            return 1;
        }

        bool hasAlpha = false;
        for (int i = 0; i < width * height; i++)
        {
            if (data[i * 4 + 3] != 0xFF)
            {
                hasAlpha = true;
                break;
            }
        }

        File998 image;
        image.width = width;
        image.height = height;
        image.bpp = hasAlpha ? 32 : 24;

        auto stride = (image.width * (image.bpp >> 3) + 3) & 0xFFFFFFFC;

        size_t decompressedSize = image.height * stride;
        image.data.resize(decompressedSize, 0);

        for (int y = 0; y < image.height; y++)
        {
            for (int x = 0; x < image.width; x++)
            {
                int srcIdx = (y * image.width + x) * 4;
                int dstIdx = (y * stride + x * (image.bpp >> 3));

                image.data[dstIdx + 0] = data[srcIdx + 2];
                image.data[dstIdx + 1] = data[srcIdx + 1];
                image.data[dstIdx + 2] = data[srcIdx + 0];

                if (hasAlpha)
                    image.data[dstIdx + 3] = data[srcIdx + 3];
            }
        }

        stbi_image_free(data);

        auto compressed = MsoCompressLZW(image.data);

        std::ofstream out(output, std::ios::binary);
        auto write = [&]<typename T>(T &value) {
            out.write(reinterpret_cast<char *>(&value), sizeof(T));
        };

        write(image.width);
        write(image.height);
        write(image.bpp);
        out.write(reinterpret_cast<char *>(compressed.data()), compressed.size());
        out.flush();
    }
}