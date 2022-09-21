CC = clang
#CC = zig cc
CFLAGS = -Wall -g
#CFLAGS = -Wall -g -DDEBUG

picoc: scan.o hashmap.o tidwall_hashmap.o picoc.o
	$(CC) $(CFLAGS) $^ -o $@

picoc_x64: scan.o hashmap.o tidwall_hashmap.o picoc_x64.o
	$(CC) $(CFLAGS) $^ -o $@

picoc0: scan.o picoc0.o
	$(CC) $(CFLAGS) $^ -o $@

picoc1: scan.o picoc1.o
	$(CC) $(CFLAGS) $^ -o $@

testscan: scan.o testscan.o
	$(CC) $(CFLAGS) $^ -o $@

scan.o testscan.o picoc.o picoc0.o picoc1.o picoc_x64.o: scan.h

hashmap.o picoc.o picoc_x64.o: hashmap.h

tidwall_hashmap.o: tidwall/tidwall_hashmap.c tidwall/tidwall_hashmap.h
	$(CC) $(CFLAGS) -c $< -o $@

all: picoc picoc0 picoc1 picoc_x64 testscan

test: all
	./testscan < example/hello.pc
	./picoc < example/hello.pc

clean:
	rm -rf *.o *~ picoc picoc0 picoc1 picoc_x64 testscan a.out

gtags:
	gtags

htags:
	htags -gasn

clean_all:
	make -s clean
	rm -rf GTAGS GRTAGS GPATH HTML

.PHONY: all test clean gtags htags clean_all
