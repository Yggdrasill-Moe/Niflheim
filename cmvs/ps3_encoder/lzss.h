#pragma once

#include <stdint.h>

size_t lzss_decode(unsigned char* input, size_t inlen, unsigned char* output, size_t outlen);
size_t lzss_encode(unsigned char* input, size_t inlen, unsigned char* output, size_t outlen);

int encode(unsigned char *inputBuf, int length, unsigned char *outputBuf);
int decode(unsigned char *inputBuf, int length, unsigned char *outputBuf);
