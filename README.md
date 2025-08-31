# File Compression Tool (RLE) in C

Simple, lossless file compression/decompression tool using a byte-oriented RLE
(run-length encoding) scheme that supports both text and binary data. Includes
Makefile builds, unit tests, and GitHub Actions CI.

## Features

- RLE compression optimized with literal and run segments
- Safe container format with header: supports pass-through when not compressible
- CLI with `--compress`/`--decompress`, input/output paths
- Make targets: `build`, `compress`, `decompress`, `test`, `clean`, `lint`
- Unit tests verifying lossless round-trips and edge cases
- CI to build, lint, and run tests on every push/PR

## Build

```
make build
```

Artifacts are placed under `bin/`. The main executable is `bin/rle`.

## Usage

```
bin/rle --compress  -i <input_file> -o <output_file>
bin/rle --decompress -i <input_file> -o <output_file>
```

Make targets (convenience):

```
make compress INPUT=path/to/input OUTPUT=path/to/output.rle
make decompress INPUT=path/to/input.rle OUTPUT=path/to/output
```

## Test

```
make test
```

## Lint

```
make lint
```

CI installs `cppcheck` and runs `make lint` in addition to building and testing.

## Format

File container format:

- Magic: `RLEC` (4 bytes)
- Version: `1` (1 byte)
- Flags: 1 byte (bit 0: 1 = RLE, 0 = stored)
- Original size: 8 bytes (little endian)
- Payload size: 8 bytes (little endian)
- Payload: either RLE stream (if flags bit0=1) or original bytes (if 0)

RLE stream encoding:

- Control byte with high bit indicating type
  - If high bit = 1: run, length = (control & 0x7F) + 1, followed by 1 byte value
  - If high bit = 0: literal, length = control + 1, followed by that many bytes

