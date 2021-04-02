CC = gcc
CFLAGS =-Wno-implicit-int -Wno-int-conversion -m32

c0_src = $(wildcard src/c0*.c)
c1_src = $(wildcard src/c1*.c)

c1_tabs = $(wildcard src/*tab.s)
c1_tabs_fixed = $(c1_tabs:%tab.s=%tab_fixed.s)

all: c0 c1

c0: $(c0_src) src/c0.h
	$(CC) $(CFLAGS) -o c0 $(c0_src)

c1: $(c1_src) $(c1_tabs_fixed) src/c1.h
	$(CC) $(CFLAGS) -o c1 $(c1_src) $(c1_tabs_fixed)

cvopt: src/cvopt.c
	$(CC) $(CFLAGS) -o cvopt src/cvopt.c

%tab_fixed.s: %tab.s cvopt
	./fix_tab.sh $< > $@

