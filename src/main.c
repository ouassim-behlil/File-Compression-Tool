#include "rle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *prog) {
    fprintf(stderr, "Usage: %s (--compress|--decompress) -i <input> -o <output>\n", prog);
}

int main(int argc, char **argv) {
    int do_compress = 0, do_decompress = 0;
    const char *in_path = NULL, *out_path = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--compress") == 0 || strcmp(argv[i], "-c") == 0) {
            do_compress = 1;
        } else if (strcmp(argv[i], "--decompress") == 0 || strcmp(argv[i], "-d") == 0) {
            do_decompress = 1;
        } else if ((strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0) && i + 1 < argc) {
            in_path = argv[++i];
        } else if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) && i + 1 < argc) {
            out_path = argv[++i];
        } else {
            usage(argv[0]);
            return 2;
        }
    }

    if ((do_compress + do_decompress) != 1 || !in_path || !out_path) {
        usage(argv[0]);
        return 2;
    }

    int rc = 0;
    if (do_compress) {
        rc = rle_compress_file(in_path, out_path);
    } else {
        rc = rle_decompress_file(in_path, out_path);
    }

    if (rc != 0) {
        fprintf(stderr, "%s failed: %d\n", do_compress ? "Compression" : "Decompression", rc);
        return 1;
    }
    return 0;
}

