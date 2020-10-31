# GNU Make file for miscellanous projects
#
# Usage: make [CLANG=Y] [W32=Y] [W64=Y] [tgt]
#
#	'W32'/'W64' = cross-compile for 32- or 64-bit Windows,  using MinGW-w64,
#      on a Linux box
#	'CLANG' = use clang instead of GCC;  Linux only
# 'CC=g++-4.8' = use that version of g++;  helpful when testing older compilers
# None of these: compile using g++ on Linux,  for Linux

UNAME=$(shell qqame)

EXE=
RM=rm -f
PREFIX  =
ADDED_EXES = grab_mpc neocp gmake2bsd
CURL=-lcurl
LUNAR_LIB = -L ~/lib -llunar

ifdef W64
	EXE=.exe
	RM=del
	PREFIX  = x86_64-w64-mingw32-g
	ADDED_EXES =
	CURL = -lurlmon
	LUNAR_LIB = -L ~/win_lib -llunar
endif

ifdef W32
	EXE=.exe
	RM=del
	PREFIX  = i686-w64-mingw32-g
	ADDED_EXES =
	CURL = -lurlmon
	LUNAR_LIB = -L ~/win_lib32 -llunar
endif

ifdef CLANG
	CC=clang
else
	CC=$(PREFIX)cc
endif

ADDED_MATH_LIB=-lm

all:  bc430$(EXE) blunder$(EXE) clock1$(EXE) css_art$(EXE) \
	csv2txt$(EXE) ellip_pt$(EXE) eop_proc$(EXE) fix_obs$(EXE) \
	gfc_xvt$(EXE) gpl$(EXE) gmake2bsd$(EXE) i2mpc$(EXE) inverf$(EXE) jpl2mpc$(EXE) \
	ktest$(EXE) mpcorbx$(EXE) mpc_extr$(EXE) mpc_sort$(EXE) \
	mpc_stat$(EXE) nofs2mpc$(EXE) peirce$(EXE) sr_plot$(EXE) \
	plot_orb$(EXE) reverser$(EXE) \
	si_print$(EXE) splottes$(EXE) vid_dump$(EXE) \
	xfer2$(EXE) xfer3$(EXE)

extras: $(ADDED_EXES) mpecer$(EXE) my_wget$(EXE) radar$(EXE)

clean:
	$(RM) archive$(EXE)
	$(RM) bc430$(EXE)
	$(RM) blunder$(EXE)
	$(RM) clock1$(EXE)
	$(RM) css_art$(EXE)
	$(RM) csv2txt$(EXE)
	$(RM) ellip_pt$(EXE)
	$(RM) eop_proc$(EXE)
	$(RM) fix_obs$(EXE)
	$(RM) getpoint$(EXE)
	$(RM) gfc_xvt$(EXE)
	$(RM) gmake2bsd$(EXE)
	$(RM) gpl$(EXE)
	$(RM) grab_mpc$(EXE)
	$(RM) i2mpc$(EXE)
	$(RM) inverf$(EXE)
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
	$(RM) neocp2$(EXE)
	$(RM) nofs2mpc$(EXE)
	$(RM) peirce$(EXE)
	$(RM) plot_els$(EXE)
	$(RM) plot_orb$(EXE)
	$(RM) pointing$(EXE)
	$(RM) radar$(EXE)
	$(RM) reverser$(EXE)
	$(RM) si_print$(EXE)
	$(RM) splottes$(EXE)
	$(RM) sr_plot$(EXE)
	$(RM) vid_dump$(EXE)
	$(RM) xfer2$(EXE)
	$(RM) xfer3$(EXE)

# Programs can be installed to ~/bin,  in which case only the current user
# can use them;  or (with root privileges) you can install them to
# /usr/local/bin for all to enjoy.

install:
ifdef GLOBAL
	cp grab_mpc /usr/local/bin
else
	cp grab_mpc $(HOME)/bin
endif

CFLAGS=-Wextra -Wall -O3 -pedantic

.cpp.o:
	$(CC) $(CFLAGS) -c $<

blunder$(EXE): blunder.cpp
	$(CC) $(CFLAGS) -o blunder$(EXE) blunder.cpp $(ADDED_MATH_LIB)

clock1$(EXE): clock1.cpp
	$(CC) $(CFLAGS) -o clock1$(EXE) clock1.cpp

css_art$(EXE): css_art.c
	$(CC) $(CFLAGS) -o css_art$(EXE) css_art.c

csv2txt$(EXE): csv2txt.c
	$(CC) $(CFLAGS) -o csv2txt$(EXE) csv2txt.c

ellip_pt$(EXE): ellip_pt.c
	$(CC) $(CFLAGS) -o ellip_pt$(EXE) ellip_pt.c $(ADDED_MATH_LIB)

eop_proc$(EXE): eop_proc.c
	$(CC) $(CFLAGS) -o eop_proc$(EXE) eop_proc.c

fix_obs$(EXE): fix_obs.cpp
	$(CC) $(CFLAGS) -o fix_obs$(EXE) fix_obs.cpp

getpoint$(EXE): getpoint.c
	$(CC) $(CFLAGS) -o getpoint$(EXE) getpoint.c

gfc_xvt$(EXE): gfc_xvt.c
	$(CC) $(CFLAGS) -o gfc_xvt$(EXE) gfc_xvt.c

gmake2bsd$(EXE): gmake2bsd.c
	$(CC) $(CFLAGS) -o gmake2bsd$(EXE) gmake2bsd.c

gpl$(EXE): gpl.c
	$(CC) $(CFLAGS) -o gpl$(EXE) gpl.c

grab_mpc$(EXE): grab_mpc.c
	$(CC) $(CFLAGS) -o grab_mpc$(EXE) grab_mpc.c -DTEST_MAIN $(CURL) $(CURLI)

i2mpc$(EXE): i2mpc.cpp
	$(CC) $(CFLAGS) -o i2mpc$(EXE) i2mpc.cpp

inverf$(EXE): inverf.c
	$(CC) $(CFLAGS) -o inverf$(EXE) inverf.c -lm

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

neocp2$(EXE): neocp2.c
	$(CC) $(CFLAGS) -o neocp2$(EXE) neocp2.c $(CURL) $(CURLI)

nofs2mpc$(EXE): nofs2mpc.cpp
	$(CC) $(CFLAGS) -o nofs2mpc$(EXE) nofs2mpc.cpp $(ADDED_MATH_LIB)

peirce$(EXE): peirce.c
	$(CC) $(CFLAGS) -o peirce$(EXE) peirce.c -DTEST_MAIN $(ADDED_MATH_LIB)

plot_els$(EXE): plot_els.c splot.cpp
	$(CC) $(CFLAGS) -o plot_els$(EXE) plot_els.c splot.cpp $(ADDED_MATH_LIB)

plot_orb$(EXE): plot_orb.c
	$(CC) $(CFLAGS) -o plot_orb$(EXE) plot_orb.c $(ADDED_MATH_LIB)

radar$(EXE): radar.c
	$(CC) $(CFLAGS) -o radar$(EXE) -I ~/include radar.c $(LUNAR_LIB) $(ADDED_MATH_LIB)

si_print$(EXE): si_print.c
	$(CC) $(CFLAGS) -o si_print$(EXE) si_print.c -DTEST_CODE $(ADDED_MATH_LIB)

splottes$(EXE): splottes.cpp splot.cpp
	$(CC) $(CFLAGS) -o splottes$(EXE) splottes.cpp splot.cpp $(ADDED_MATH_LIB)

sr_plot$(EXE): sr_plot.cpp splot.cpp
	$(CC) $(CFLAGS) -o sr_plot$(EXE) sr_plot.cpp splot.cpp $(ADDED_MATH_LIB)

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

pointing : pointing.c
	$(CC) $(CFLAGS) -I /usr/include/wcstools -o pointing $< -lwcstools
