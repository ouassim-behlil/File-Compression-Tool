#include "../src/rle.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/types.h>

static void expect_roundtrip_bytes(const uint8_t *data, size_t len) {
    uint8_t *comp = NULL, *decomp = NULL;
    size_t clen = 0, dlen = 0;
    if (len > 0) {
        clen = rle_compress(data, len, &comp);
        assert((clen > 0) && comp != NULL);
    }
    dlen = len > 0 ? rle_decompress(comp, clen, &decomp) : 0;
    if (len == 0) {
        assert(dlen == 0);
    } else {
        assert(decomp != NULL);
        assert(dlen == len);
        assert(memcmp(data, decomp, len) == 0);
    }
    free(comp);
    free(decomp);
}

static void test_empty() {
    uint8_t *out = NULL;
    size_t clen = rle_compress(NULL, 0, &out);
    assert(clen == 0);
    assert(out == NULL);
}

static void test_literals_and_runs() {
    // Pattern: literals then a long run
    uint8_t data[256];
    for (int i = 0; i < 64; ++i) data[i] = (uint8_t)i; // literals
    for (int i = 64; i < 256; ++i) data[i] = 0xAA;      // run
    expect_roundtrip_bytes(data, sizeof(data));
}

static void test_alternating() {
    uint8_t data[257];
    for (int i = 0; i < 257; ++i) data[i] = (uint8_t)(i & 1 ? 0x00 : 0xFF);
    expect_roundtrip_bytes(data, sizeof(data));
}

static void test_long_run_boundaries() {
    // runs > 128 are split; ensure decode matches
    size_t len = 400;
    uint8_t *data = (uint8_t *)malloc(len);
    memset(data, 7, len);
    expect_roundtrip_bytes(data, len);
    free(data);
}

static void gen_tmp_path(const char *prefix, char *path_out, size_t path_cap) {
    int mkrc = mkdir("build", 0777);
    if (mkrc != 0 && errno != EEXIST) {
        fprintf(stderr, "Failed to create build directory: %d\n", errno);
        assert(0 && "mkdir build failed");
    }
    unsigned r = (unsigned)rand();
    pid_t pid = getpid();
    snprintf(path_out, path_cap, "build/%s_%d_%u.bin", prefix, (int)pid, r);
}

static void test_file_roundtrip_binary() {
    // Create binary with zeros, randoms, and repeats
    size_t len = 1024;
    uint8_t *data = (uint8_t *)malloc(len);
    srand(1234);
    for (size_t i = 0; i < len; ++i) data[i] = (uint8_t)rand();
    for (size_t i = 200; i < 400; ++i) data[i] = 0x55; // ensure compressible region

    // Write input
    char in_path[256], comp_path[256], out_path[256];
    gen_tmp_path("in", in_path, sizeof(in_path));
    FILE *f = fopen(in_path, "wb");
    assert(f);
    assert(fwrite(data, 1, len, f) == len);
    fclose(f);

    // Compress and decompress
    gen_tmp_path("c", comp_path, sizeof(comp_path));
    gen_tmp_path("out", out_path, sizeof(out_path));
    assert(rle_compress_file(in_path, comp_path) == 0);
    assert(rle_decompress_file(comp_path, out_path) == 0);

    // Read output
    FILE *fo = fopen(out_path, "rb");
    assert(fo);
    uint8_t *out = (uint8_t *)malloc(len);
    assert(fread(out, 1, len, fo) == len);
    fclose(fo);

    assert(memcmp(out, data, len) == 0);
    free(out);
    free(data);
    remove(in_path);
    remove(comp_path);
    remove(out_path);
}

int main(void) {
    test_empty();
    test_literals_and_runs();
    test_alternating();
    test_long_run_boundaries();
    test_file_roundtrip_binary();
    printf("All tests passed.\n");
    return 0;
}
