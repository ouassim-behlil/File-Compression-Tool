#include "rle.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define RLE_MAGIC "RLEC"
#define RLE_VERSION 1

static void write_le64(uint8_t out[8], uint64_t v) {
    for (int i = 0; i < 8; ++i) out[i] = (uint8_t)((v >> (8 * i)) & 0xFF);
}

static uint64_t read_le64(const uint8_t in[8]) {
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= ((uint64_t)in[i]) << (8 * i);
    return v;
}

// Ensure capacity for dynamic buffer
static int ensure_capacity(uint8_t **buf, size_t *cap, size_t needed) {
    if (needed <= *cap) return 1;
    size_t new_cap = *cap ? *cap : 64;
    while (new_cap < needed) {
        new_cap = new_cap < (SIZE_MAX / 2) ? new_cap * 2 : SIZE_MAX;
        if (new_cap == SIZE_MAX && new_cap < needed) return 0;
    }
    uint8_t *nb = (uint8_t *)realloc(*buf, new_cap);
    if (!nb) return 0;
    *buf = nb;
    *cap = new_cap;
    return 1;
}

size_t rle_compress(const uint8_t *in, size_t in_size, uint8_t **out_buf) {
    *out_buf = NULL;
    size_t out_size = 0, out_cap = 0;

    if (in_size == 0) {
        // Empty input -> empty output
        return 0;
    }

    // Process using literal and run blocks
    size_t i = 0;
    while (i < in_size) {
        // Detect run of repeated bytes starting at i
        size_t run_len = 1;
        while (i + run_len < in_size && in[i + run_len] == in[i] && run_len < 128) {
            run_len++;
        }
        if (run_len >= 2) {
            // Emit run block: control with high bit set
            if (!ensure_capacity(out_buf, &out_cap, out_size + 2)) {
                free(*out_buf);
                *out_buf = NULL;
                return 0;
            }
            uint8_t ctrl = (uint8_t)(0x80 | (uint8_t)(run_len - 1));
            (*out_buf)[out_size++] = ctrl;
            (*out_buf)[out_size++] = in[i];
            i += run_len;
        } else {
            // Build literal run up to 128 or until a run of >=2 would start
            size_t lit_start = i;
            size_t lit_len = 1;
            i++;
            while (i < in_size && lit_len < 128) {
                // Check if a run of >=2 starts at i; if so, stop literal
                size_t r = 1;
                while (i + r < in_size && in[i + r] == in[i] && r < 128) r++;
                if (r >= 2) break;
                lit_len++;
                i++;
            }
            if (!ensure_capacity(out_buf, &out_cap, out_size + 1 + lit_len)) {
                free(*out_buf);
                *out_buf = NULL;
                return 0;
            }
            uint8_t ctrl = (uint8_t)(lit_len - 1); // high bit 0
            (*out_buf)[out_size++] = ctrl;
            memcpy(*out_buf + out_size, in + lit_start, lit_len);
            out_size += lit_len;
        }
    }

    return out_size;
}

size_t rle_decompress(const uint8_t *in, size_t in_size, uint8_t **out_buf) {
    *out_buf = NULL;
    size_t out_size = 0, out_cap = 0;

    size_t i = 0;
    while (i < in_size) {
        uint8_t ctrl = in[i++];
        if (ctrl & 0x80) {
            size_t len = (size_t)(ctrl & 0x7F) + 1;
            if (i >= in_size) {
                free(*out_buf);
                *out_buf = NULL;
                return 0; // malformed
            }
            uint8_t val = in[i++];
            if (!ensure_capacity(out_buf, &out_cap, out_size + len)) {
                free(*out_buf);
                *out_buf = NULL;
                return 0;
            }
            memset(*out_buf + out_size, val, len);
            out_size += len;
        } else {
            size_t len = (size_t)ctrl + 1;
            if (i + len > in_size) {
                free(*out_buf);
                *out_buf = NULL;
                return 0; // malformed
            }
            if (!ensure_capacity(out_buf, &out_cap, out_size + len)) {
                free(*out_buf);
                *out_buf = NULL;
                return 0;
            }
            memcpy(*out_buf + out_size, in + i, len);
            i += len;
            out_size += len;
        }
    }

    return out_size;
}

static int read_entire_file(const char *path, uint8_t **data, size_t *len) {
    *data = NULL; *len = 0;
    FILE *f = fopen(path, "rb");
    if (!f) return errno ? errno : -1;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -1; }
    long sz = ftell(f);
    if (sz < 0) { fclose(f); return -1; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return -1; }
    if (sz == 0) { fclose(f); *data = NULL; *len = 0; return 0; }
    *data = (uint8_t *)malloc((size_t)sz);
    if (!*data) { fclose(f); return -1; }
    size_t rd = fread(*data, 1, (size_t)sz, f);
    fclose(f);
    if (rd != (size_t)sz) { free(*data); *data = NULL; return -1; }
    *len = rd;
    return 0;
}

static int write_entire_file(const char *path, const uint8_t *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return errno ? errno : -1;
    size_t wr = fwrite(data, 1, len, f);
    fclose(f);
    return wr == len ? 0 : -1;
}

int rle_compress_file(const char *input_path, const char *output_path) {
    uint8_t *in = NULL, *compressed = NULL;
    size_t in_len = 0;
    int rc = read_entire_file(input_path, &in, &in_len);
    if (rc != 0) return rc;

    size_t comp_len = 0;
    if (in_len > 0) comp_len = rle_compress(in, in_len, &compressed);
    if (in_len > 0 && comp_len == 0 && in != NULL) { free(in); return -1; }

    // Prepare output buffer with header
    uint8_t flags = 0;
    const uint8_t *payload = in;
    size_t payload_len = in_len;
    if (comp_len > 0 && comp_len < in_len) {
        flags = 0x01; // RLE
        payload = compressed;
        payload_len = comp_len;
    }

    size_t total_len = 4 + 1 + 1 + 8 + 8 + payload_len;
    uint8_t *out = (uint8_t *)malloc(total_len);
    if (!out) { free(in); free(compressed); return -1; }
    memcpy(out, RLE_MAGIC, 4);
    out[4] = RLE_VERSION;
    out[5] = flags;
    write_le64(out + 6, (uint64_t)in_len);
    write_le64(out + 14, (uint64_t)payload_len);
    if (payload_len)
        memcpy(out + 22, payload, payload_len);

    rc = write_entire_file(output_path, out, total_len);
    free(out);
    free(in);
    free(compressed);
    return rc;
}

int rle_decompress_file(const char *input_path, const char *output_path) {
    uint8_t *in = NULL;
    size_t in_len = 0;
    int rc = read_entire_file(input_path, &in, &in_len);
    if (rc != 0) return rc;
    if (in_len < 22) { free(in); return -1; }
    if (memcmp(in, RLE_MAGIC, 4) != 0) { free(in); return -1; }
    if (in[4] != RLE_VERSION) { free(in); return -1; }
    uint8_t flags = in[5];
    uint64_t orig_size = read_le64(in + 6);
    uint64_t payload_size = read_le64(in + 14);
    if (22 + payload_size != in_len) { free(in); return -1; }

    int res = 0;
    if ((flags & 0x01) == 0) {
        // stored
        res = write_entire_file(output_path, in + 22, (size_t)payload_size);
        if (res == 0 && (size_t)orig_size != (size_t)payload_size) {
            // size mismatch indicates corruption
            res = -1;
        }
        free(in);
        return res;
    }

    uint8_t *out = NULL;
    size_t out_len = rle_decompress(in + 22, (size_t)payload_size, &out);
    free(in);
    if (out_len == 0 && orig_size != 0) { free(out); return -1; }
    if ((uint64_t)out_len != orig_size) { free(out); return -1; }
    res = write_entire_file(output_path, out, out_len);
    free(out);
    return res;
}

