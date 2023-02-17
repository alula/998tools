#include "compress.h"

static void printCode(MSOLZWDICT *dictionary, int dictSize, uint16_t code)
{
    {
        std::vector<uint16_t> dcodes;
        // print dictionary for specific code
        printf("[ ");
        uint16_t c = code;
        while (c != 0xffff)
        {
            dcodes.insert(dcodes.begin(), dictionary[c].suffix);
            c = dictionary[c].prefix;
        }
        for (auto c : dcodes)
            printf("%02x ", c);
        printf("] ");
    }
}

static std::vector<uint16_t> codes;

void MsoUncompressLZW(std::span<uint8_t> const &input, std::span<uint8_t> const &output)
{
    MSOLZWDICT dictionary[4096];

    for (int i = 0; i < 256; i++)
    {
        dictionary[i].suffix = i;
        dictionary[i].prefix = 0xffff;
        dictionary[i].prefixLength = 0;
    }

    const checked_span<uint8_t> inData(input);
    checked_span<uint8_t> outData(output);

    int iInDataLen = (int)inData.size();
    int iOutDataLen = (int)outData.size();

    int outLen = iOutDataLen - 1;
    outData.at(0) = inData.at(0);

    int pRead = 1;
    int pWrite = 1;
    int codeBits = 8;
    uint16_t dictSize = 256;
    int shiftBits = 8;

    // codes.push_back(*pInData);

    for (uint16_t code = inData.at(0); iInDataLen > 1; ++pWrite)
    {
        // printCode(dictionary, dictSize, code);
        // printf("code: %#04x\n", code);
        codes.push_back(code);

        int inLen = iInDataLen;
        if (outLen <= 0)
        {
            // printf("outLen <= 0\n");
            return;
        }

        uint16_t prevCode = code;

        if (((uint16_t)(dictSize - 1) & dictSize) == 0)
        {
            if (dictSize >= 0x1000u)
            {
                // printf("clear\n");
                // uint16_t v24 = inData.at(pRead);
                code = inData.at(pRead);
                --outLen;
                ++pRead;
                // *pWrite = v24;
                outData.at(pWrite) = (uint8_t)code;
                --iInDataLen;
                codeBits = 8;
                dictSize = 256;
                continue;
            }
            codeBits++;
        }

        --iInDataLen;
        uint16_t codeHighBits = inData.at(pRead++) << (codeBits - shiftBits);
        uint16_t codeLowBits;

        if (shiftBits + 8 <= codeBits)
        {
            iInDataLen = inLen - 2;
            if (inLen - 2 < 1)
                return;

            if (pRead >= inData.size())
                return;
            uint16_t codeHighBits = inData.at(pRead++) << (codeBits - shiftBits - 8);
            if (pRead >= inData.size())
                return;

            shiftBits += 16 - codeBits;
            codeLowBits = (uint8_t)(inData.at(pRead) >> shiftBits) | codeHighBits;
        }
        else
        {
            shiftBits += 8 - codeBits;
            codeLowBits = (uint8_t)(inData.at(pRead) >> shiftBits);
        }

        code = ((1 << codeBits) - 1) & (codeHighBits | codeLowBits);

        if (code == dictSize)
        {
            dictionary[dictSize].prefixLength = (uint16_t)(dictionary[prevCode].prefixLength + 1);
            int v26 = -dictionary[dictSize].prefixLength;
            if (v26 < outLen)
            {
                dictionary[dictSize].prefix = prevCode;
                dictionary[dictSize].suffix = outData.at(pWrite + v26);
            }
        }

        int stringLen = dictionary[code].prefixLength;
        // uint8_t *stringWritePtr = &pWrite[stringLen];
        int stringWritePtr = pWrite + stringLen;
        stringLen = outLen - stringLen;
        outLen = stringLen - 1;
        pWrite = stringWritePtr;
        if (stringLen <= 0)
        {
            // printf("stringLen <= 0\n");
            return;
        }

        uint16_t suffix = code;
        int len = iOutDataLen - outLen;
        do
        {
            if (len > 0)
            {
                --len;
                outData.at(stringWritePtr--) = (uint8_t)dictionary[suffix].suffix;
                suffix = dictionary[suffix].prefix;
            }
        } while (suffix != 0xFFFF);

        if (code != dictSize && len <= iOutDataLen)
        {
            dictionary[dictSize].suffix = outData.at(stringWritePtr + 1);
            dictionary[dictSize].prefix = prevCode;
            dictionary[dictSize].prefixLength = dictionary[prevCode].prefixLength + 1;
        }
        ++dictSize;
    }

    // printf("end\n");
}

std::vector<uint8_t> MsoCompressLZW(std::span<uint8_t> const &input)
{
    checked_span<uint8_t> inData(input);
    std::vector<uint8_t> outData;
    size_t bitIndex = 0;

    // append a specified amount of bits to the output buffer (MSB-first)
    auto appendBits = [&](uint16_t value, uint8_t bitCount)
    {
        for (int i = bitCount - 1; i >= 0; i--)
        {
            if (bitIndex % 8 == 0)
                outData.push_back(0);

            if (value & (1 << i))
                outData[bitIndex / 8] |= (1 << (7 - (bitIndex % 8)));

            bitIndex++;
        }
    };

    // append a byte aligned value to the output buffer and align the bit index
    auto appendByte = [&](uint8_t value)
    {
        outData.push_back(value);
        bitIndex = outData.size() * 8;
    };

    MSOLZWDICT dictionary[4096];

    for (int i = 0; i < 256; i++)
    {
        dictionary[i].suffix = i;
        dictionary[i].prefix = 0xffff;
        dictionary[i].prefixLength = 0;
    }

    constexpr uint8_t CLEAR_CODE = 0x00;
    std::vector<uint8_t> buffer;

    // appendByte(CLEAR_CODE);

    buffer.push_back(inData.at(0));
    // appendByte(inData.at(0));

    int dictSize = 256;
    int codeBits = 8;

    // find a dictionary entry that matches the current buffer
    auto findCode = [&]()
    {
        for (int i = 0; i < dictSize; i++)
        {
            if ((dictionary[i].prefixLength + 1) == buffer.size())
            {
                bool match = true;
                int dictIdx = i;
                for (int j = (int)buffer.size() - 1; j >= 0; j--)
                {
                    if (dictionary[dictIdx].suffix == buffer[j])
                    {
                        dictIdx = dictionary[dictIdx].prefix;
                    }
                    else
                    {
                        match = false;
                        break;
                    }
                }

                if (match)
                {
                    // printf("Found matching code %#04x len=%d bufsize=%d\n", i, dictionary[i].prefixLength + 1, buffer.size());
                    return i;
                }
            }
        }

        return -1;
    };

    int codeIdxDebug = 0;
    int prevCode = inData.at(0);
    int maxSequenceLen = 0;

    for (int i = 1; i < (int)inData.size(); i++)
    {
        int remaining = (int)inData.size() - i - 2;
        int takeBytes = std::min(remaining, maxSequenceLen) + 1;
        int startI = i;

        for (int j = 0; j < takeBytes; j++)
        {
            buffer.push_back(inData.at(i++));
        }

        do
        {
            int c = findCode();
            if (c != -1)
            {
                prevCode = c;
                break;
            }
            else if (buffer.size() > 1)
            {
                i--;
                buffer.pop_back();
            }
            else
            {
                break;
            }
        } while (i > startI);

        uint8_t c = inData.at(i);
        buffer.push_back(c);
        int code = findCode();
        if (code == -1)
        {
            buffer.pop_back();
            code = prevCode;

            // {
            //     printf("[ ");
            //     for (int k = 0; k < buffer.size(); k++)
            //     {
            //         printf("%02x ", buffer[k]);
            //     }
            //     printf("] ");
            // }

            buffer.clear();
            buffer.push_back(c);

            // printf("emit code: %#04x\n", code);
            appendBits(code, codeBits);

            // if (codes[codeIdxDebug++] != code)
            // {
            //     printCode(dictionary, dictSize, codes[codeIdxDebug - 1]);
            //     printf("prefix len: %d %d\n", dictionary[code].prefixLength, dictionary[codes[codeIdxDebug - 1]].prefixLength);
            //     printf("code mismatch %#04x != %#04x\n", codes[codeIdxDebug - 1], code);
            //     throw std::exception();
            // }

            dictionary[dictSize].suffix = c;
            dictionary[dictSize].prefix = code;
            dictionary[dictSize].prefixLength = dictionary[code].prefixLength + 1;

            if (dictionary[dictSize].prefixLength > maxSequenceLen)
                maxSequenceLen = dictionary[dictSize].prefixLength;

            dictSize++;

            if (dictSize > 0x1000u)
            {
                dictSize = 256;
                codeBits = 8;
                maxSequenceLen = 0;
            }

            code = c;
        }

        prevCode = code;

        if (dictSize > (1 << codeBits))
            codeBits++;
    }

    if (!buffer.empty())
    {
        int dictIdx = findCode();
        if (dictIdx != -1)
        {
            // printf("emit code: %#04x\n", dictIdx);
            appendBits(dictIdx, codeBits);
        }
        else
        {
            throw std::runtime_error("Couldn't find dictionary entry for tail.");
        }
    }

    return outData;
}
