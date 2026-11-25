CC = gcc
CPPFLAGS = -D_POSIX_C_SOURCE=200809L
CFLAGS = -std=c11 -O2 -g -Wall -Wextra -Wpedantic -Wshadow -Wstrict-prototypes
LDLIBS = -lm
SOURCES = grafo.c prim.c binomial_heap.c fibonacci_heap.c cluster.c experiments.c
HEADERS = grafo.h binomial_heap.h fibonacci_heap.h cluster.h experiments.h
ASAN_OPTIONS ?= detect_leaks=1:halt_on_error=1:exitcode=99
UBSAN_OPTIONS ?= halt_on_error=1:exitcode=99
SANFLAGS = -std=c11 -O1 -g3 -Wall -Wextra -Wpedantic -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie

ZIP_NAME = tarea1.zip
ZIP_FILES = README.md Makefile run_experiments.sh main.c tests.c tests_cli.sh $(SOURCES) $(HEADERS)

all: prim_mst

prim_mst: main.c $(SOURCES) $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ main.c $(SOURCES) $(LDLIBS)

test_mst: tests.c $(SOURCES) $(HEADERS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ tests.c $(SOURCES) $(LDLIBS)

test: prim_mst test_mst
	./test_mst
	sh tests_cli.sh ./prim_mst

sanitize: tests.c main.c $(SOURCES) $(HEADERS)
	$(CC) $(CPPFLAGS) $(SANFLAGS) -o test_mst_san tests.c $(SOURCES) $(LDLIBS)
	$(CC) $(CPPFLAGS) $(SANFLAGS) -o prim_mst_san main.c $(SOURCES) $(LDLIBS)
	ASAN_OPTIONS=$(ASAN_OPTIONS) UBSAN_OPTIONS=$(UBSAN_OPTIONS) ./test_mst_san
	ASAN_OPTIONS=$(ASAN_OPTIONS) UBSAN_OPTIONS=$(UBSAN_OPTIONS) sh tests_cli.sh ./prim_mst_san

experiments: prim_mst
	./run_experiments.sh

valgrind: test_mst
	valgrind --leak-check=full --show-leak-kinds=all --errors-for-leak-kinds=all --error-exitcode=1 ./test_mst

clean:
	rm -f prim_mst test_mst test_mst_san prim_mst_san $(ZIP_NAME)

zip: clean
	zip -r $(ZIP_NAME) $(ZIP_FILES)

.PHONY: all test sanitize valgrind experiments clean zip
