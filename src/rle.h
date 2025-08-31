#ifndef RLE_H
#define RLE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t rle_compress(const uint8_t *in, size_t in_size, uint8_t **out_buf);

size_t rle_decompress(const uint8_t *in, size_t in_size, uint8_t **out_buf);

int rle_compress_file(const char *input_path, const char *output_path);
int rle_decompress_file(const char *input_path, const char *output_path);

#ifdef __cplusplus
}
#endif

#endif
