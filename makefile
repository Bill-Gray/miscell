UNAME=$(shell qqame)

ifdef MSWIN
	EXE=.exe
	RM=del
	PREFIX  = x86_64-w64-mingw32-
	ADDED_EXES =
else
	EXE=
	RM=rm -f
	PREFIX  =
	ADDED_EXES = grab_bmp grab_mpc neocp
endif

ifdef CLANG
CC=clang -Weverything
else
CC=$(PREFIX)g++
endif

all:   blunder$(EXE) clock1$(EXE) fix_obs$(EXE)   \
	i2mpc$(EXE) jpl2mpc$(EXE) ktest$(EXE) nofs2mpc$(EXE) \
	plot_orb$(EXE) si_print$(EXE)	splottes$(EXE) \
	xfer2$(EXE) xfer3$(EXE) $(ADDED_EXES)


clean:
	$(RM) blunder$(EXE)
	$(RM) clock1$(EXE)
	$(RM) fix_obs$(EXE)
	$(RM) grab_bmp$(EXE)
	$(RM) grab_mpc$(EXE)
	$(RM) i2mpc$(EXE)
	$(RM) jpl2mpc$(EXE)
	$(RM) ktest$(EXE)
	$(RM) neocp$(EXE)
	$(RM) nofs2mpc$(EXE)
	$(RM) plot_orb$(EXE)
	$(RM) si_print$(EXE)
	$(RM) splottes$(EXE)
	$(RM) xfer2$(EXE)
	$(RM) xfer3$(EXE)

CFLAGS=-Wextra -Wall -O3 -pedantic

.cpp.o:
	$(CC) $(CFLAGS) -c $<

blunder$(EXE): blunder.cpp
	$(CC) $(CFLAGS) -o blunder$(EXE) blunder.cpp -lm

clock1$(EXE): clock1.cpp
	echo $(INSTALL_PATH)
	$(CC) $(CFLAGS) -o clock1$(EXE) clock1.cpp

grab_bmp$(EXE): grab_bmp.cpp
	$(CC) $(CFLAGS) -o grab_bmp$(EXE) grab_bmp.cpp

fix_obs$(EXE): fix_obs.cpp
	$(CC) $(CFLAGS) -o fix_obs$(EXE) fix_obs.cpp

grab_mpc$(EXE): grab_mpc.c
	$(CC) $(CFLAGS) -o grab_mpc$(EXE) grab_mpc.c -lcurl

i2mpc$(EXE): i2mpc.cpp
	$(CC) $(CFLAGS) -o i2mpc$(EXE) i2mpc.cpp

ktest$(EXE): ktest.c
	$(CC) $(CFLAGS) -o ktest$(EXE) ktest.c -lm

jpl2mpc$(EXE): jpl2mpc.cpp
	$(CC) $(CFLAGS) -o jpl2mpc$(EXE) jpl2mpc.cpp

neocp$(EXE): neocp.c
	$(CC) $(CFLAGS) -o neocp$(EXE) neocp.c -lcurl

nofs2mpc$(EXE): nofs2mpc.cpp
	$(CC) $(CFLAGS) -o nofs2mpc$(EXE) nofs2mpc.cpp -lm

plot_orb$(EXE): plot_orb.c
	$(CC) $(CFLAGS) -o plot_orb$(EXE) plot_orb.c -lm

si_print$(EXE): si_print.c
	$(CC) $(CFLAGS) -DTEST_CODE -o si_print$(EXE) si_print.c

splottes$(EXE): splottes.cpp splot.cpp
	$(CC) $(CFLAGS) -o splottes$(EXE) splottes.cpp splot.cpp

xfer2$(EXE): xfer2.cpp
	$(CC) $(CFLAGS) -o xfer2$(EXE) xfer2.cpp -lm

xfer3$(EXE): xfer3.cpp
	$(CC) $(CFLAGS) -o xfer3$(EXE) xfer3.cpp -lm

z1$(EXE): z1.cpp
	$(CC) $(CFLAGS) -o z1$(EXE) z1.cpp -lcurses

geo_test$(EXE): geo_test.cpp geo_pot.cpp
	$(CC) $(CFLAGS) -o geo_test$(EXE) geo_test.cpp geo_pot.cpp
