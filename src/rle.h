#ifndef RLE_H
#define RLE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// In-memory RLE compress: returns size of output; *out_buf is malloc'd and must be freed by caller.
// Returns 0 on failure (and *out_buf == NULL).
size_t rle_compress(const uint8_t *in, size_t in_size, uint8_t **out_buf);

// In-memory RLE decompress: returns size of output; *out_buf is malloc'd and must be freed by caller.
// Returns 0 on failure (and *out_buf == NULL).
size_t rle_decompress(const uint8_t *in, size_t in_size, uint8_t **out_buf);

// File helpers with header container. Return 0 on success, non-zero on error.
int rle_compress_file(const char *input_path, const char *output_path);
int rle_decompress_file(const char *input_path, const char *output_path);

#ifdef __cplusplus
}
#endif

#endif // RLE_H

