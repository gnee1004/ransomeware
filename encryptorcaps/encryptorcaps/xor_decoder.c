#include "xor_decoder.h"

void xor_decrypt(char* data, size_t len, char key)
{
    for (size_t i = 0; i < len; ++i) data[i] ^= key;
}

void xor_decrypt_wide(wchar_t* data, size_t len, wchar_t key)
{
    unsigned char* bytes = (unsigned char*)data;
    for (size_t i = 0; i < len * sizeof(wchar_t); ++i)
        bytes[i] ^= (unsigned char)key;
}
