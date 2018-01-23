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
	ADDED_EXES = grab_mpc neocp gmake2bsd
endif

ifdef CLANG
CC=clang
else
CC=$(PREFIX)cc
endif

ADDED_MATH_LIB=-lm
CURL=-lcurl

all:   blunder$(EXE) clock1$(EXE) csv2txt$(EXE) ellip_pt$(EXE) fix_obs$(EXE) \
	eop_proc$(EXE) i2mpc$(EXE) jpl2mpc$(EXE) ktest$(EXE) mpcorbx$(EXE) \
	mpecer$(EXE) mpc_extr$(EXE) mpc_sort$(EXE) \
	mpc_stat$(EXE) my_wget$(EXE) nofs2mpc$(EXE) \
	plot_orb$(EXE) radar$(EXE) si_print$(EXE) splottes$(EXE) vid_dump$(EXE) \
	xfer2$(EXE) xfer3$(EXE) $(ADDED_EXES)


clean:
	$(RM) blunder$(EXE)
	$(RM) clock1$(EXE)
	$(RM) csv2txt$(EXE)
	$(RM) ellip_pt$(EXE)
	$(RM) eop_proc$(EXE)
	$(RM) fix_obs$(EXE)
	$(RM) gmake2bsd$(EXE)
	$(RM) grab_mpc$(EXE)
	$(RM) i2mpc$(EXE)
	$(RM) jpl2mpc$(EXE)
	$(RM) ktest$(EXE)
	$(RM) mpc_stat$(EXE)
	$(RM) mpc_extr$(EXE)
	$(RM) mpc_sort$(EXE)
	$(RM) mpc_up$(EXE)
	$(RM) mpcorbx$(EXE)
	$(RM) mpecer$(EXE)
	$(RM) my_wget$(EXE)
	$(RM) neocp$(EXE)
	$(RM) nofs2mpc$(EXE)
	$(RM) plot_orb$(EXE)
	$(RM) radar$(EXE)
	$(RM) si_print$(EXE)
	$(RM) splottes$(EXE)
	$(RM) vid_dump$(EXE)
	$(RM) xfer2$(EXE)
	$(RM) xfer3$(EXE)

CFLAGS=-Wextra -Wall -O3 -pedantic

.cpp.o:
	$(CC) $(CFLAGS) -c $<

blunder$(EXE): blunder.cpp
	$(CC) $(CFLAGS) -o blunder$(EXE) blunder.cpp $(ADDED_MATH_LIB)

clock1$(EXE): clock1.cpp
	$(CC) $(CFLAGS) -o clock1$(EXE) clock1.cpp

csv2txt$(EXE): csv2txt.c
	$(CC) $(CFLAGS) -o csv2txt$(EXE) csv2txt.c

ellip_pt$(EXE): ellip_pt.c
	$(CC) $(CFLAGS) -o ellip_pt$(EXE) ellip_pt.c $(ADDED_MATH_LIB)

eop_proc$(EXE): eop_proc.c
	$(CC) $(CFLAGS) -o eop_proc$(EXE) eop_proc.c

fix_obs$(EXE): fix_obs.cpp
	$(CC) $(CFLAGS) -o fix_obs$(EXE) fix_obs.cpp

gmake2bsd$(EXE): gmake2bsd.c
	$(CC) $(CFLAGS) -o gmake2bsd$(EXE) gmake2bsd.c

grab_mpc$(EXE): grab_mpc.c
	$(CC) $(CFLAGS) -o grab_mpc$(EXE) grab_mpc.c -DTEST_MAIN $(CURL) $(CURLI)

i2mpc$(EXE): i2mpc.cpp
	$(CC) $(CFLAGS) -o i2mpc$(EXE) i2mpc.cpp

ktest$(EXE): ktest.c
	$(CC) $(CFLAGS) -o ktest$(EXE) ktest.c $(ADDED_MATH_LIB)

jpl2mpc$(EXE): jpl2mpc.cpp
	$(CC) $(CFLAGS) -o jpl2mpc$(EXE) jpl2mpc.cpp

mpc_stat$(EXE): mpc_stat.cpp
	$(CC) $(CFLAGS) -o mpc_stat$(EXE) mpc_stat.cpp $(ADDED_MATH_LIB)

mpc_extr$(EXE): mpc_extr.cpp
	$(CC) $(CFLAGS) -o mpc_extr$(EXE) mpc_extr.cpp

mpc_sort$(EXE): mpc_sort.cpp
	$(CC) $(CFLAGS) -o mpc_sort$(EXE) mpc_sort.cpp

mpc_up$(EXE): mpc_up.c
	$(CC) $(CFLAGS) -o mpc_up$(EXE) mpc_up.c

mpecer$(EXE): mpecer.c
	$(CC) $(CFLAGS) -o mpecer$(EXE) mpecer.c $(CURL) $(CURLI)

mpcorbx$(EXE): mpcorbx.c
	$(CC) $(CFLAGS) -o mpcorbx$(EXE) mpcorbx.c -lm

my_wget$(EXE): my_wget.c
	$(CC) $(CFLAGS) -o my_wget$(EXE) my_wget.c $(CURL) $(CURLI) -lpthread

neocp$(EXE): neocp.c
	$(CC) $(CFLAGS) -o neocp$(EXE) neocp.c $(CURL) $(CURLI)

nofs2mpc$(EXE): nofs2mpc.cpp
	$(CC) $(CFLAGS) -o nofs2mpc$(EXE) nofs2mpc.cpp $(ADDED_MATH_LIB)

plot_orb$(EXE): plot_orb.c
	$(CC) $(CFLAGS) -o plot_orb$(EXE) plot_orb.c $(ADDED_MATH_LIB)

radar$(EXE): radar.c
	$(CC) $(CFLAGS) -o radar$(EXE) radar.c

si_print$(EXE): si_print.c
	$(CC) $(CFLAGS) -o si_print$(EXE) si_print.c -DTEST_CODE $(ADDED_MATH_LIB)

splottes$(EXE): splottes.cpp splot.cpp
	$(CC) $(CFLAGS) -o splottes$(EXE) splottes.cpp splot.cpp $(ADDED_MATH_LIB)

vid_dump$(EXE): vid_dump.c
	$(CC) $(CFLAGS) -o vid_dump$(EXE) vid_dump.c

xfer2$(EXE): xfer2.cpp
	$(CC) $(CFLAGS) -o xfer2$(EXE) xfer2.cpp $(ADDED_MATH_LIB)

xfer3$(EXE): xfer3.cpp
	$(CC) $(CFLAGS) -o xfer3$(EXE) xfer3.cpp $(ADDED_MATH_LIB)

z1$(EXE): z1.cpp
	$(CC) $(CFLAGS) -o z1$(EXE) z1.cpp -lcurses

geo_test$(EXE): geo_test.cpp geo_pot.cpp
	$(CC) $(CFLAGS) -o geo_test$(EXE) geo_test.cpp geo_pot.cpp
