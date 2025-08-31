CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror
LDFLAGS ?=

SRC := src/rle.c src/main.c
OBJ := $(patsubst %.c,build/%.o,$(SRC))
TEST_SRC := tests/test_rle.c
TEST_OBJ := $(patsubst %.c,build/%.o,$(TEST_SRC))

.PHONY: all build test clean compress decompress lint

all: build

build: bin/rle

bin/rle: $(OBJ)
	@mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

build/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I src -c -o $@ $<

test: bin/rle build/tests/test_rle
	build/tests/test_rle

build/tests/test_rle: $(TEST_OBJ) build/src/rle.o
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

compress: bin/rle
	@if [ -z "$(INPUT)" ] || [ -z "$(OUTPUT)" ]; then \
		echo "Usage: make compress INPUT=path OUTPUT=path"; exit 2; \
	fi
	bin/rle --compress -i "$(INPUT)" -o "$(OUTPUT)"

decompress: bin/rle
	@if [ -z "$(INPUT)" ] || [ -z "$(OUTPUT)" ]; then \
		echo "Usage: make decompress INPUT=path OUTPUT=path"; exit 2; \
	fi
	bin/rle --decompress -i "$(INPUT)" -o "$(OUTPUT)"

lint:
	@which cppcheck >/dev/null 2>&1 && \
	  cppcheck --enable=warning,style,performance,portability --std=c11 --force \
	  --inline-suppr -I src --quiet src tests || \
	  echo "cppcheck not found; skipping static analysis"

clean:
	rm -rf build bin

