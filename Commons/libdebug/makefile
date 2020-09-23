#
# LIB = library name
# SRC = list of source files the library is made of.
# LOCAL_CFLAGS = list of private Compiler flags
# WARNING : if library is a local one, comment the line after install:
# ( Do not install in project lib dir )
#

LIB = libdebug.a
LIBD = libdebug.d.a
SRC = main.c
LOCAL_CFLAGS =

.SUFFIXES: .uil .uid .do

include	${PROJECT_HOME}/Commons/makefiles/makefile.libstd
# DO NOT DELETE THIS LINE -- make depend depends on it.

main.o: /usr/include/fcntl.h /usr/include/features.h
main.o: /usr/include/stdc-predef.h /usr/include/stdio.h /usr/include/string.h
main.o: /usr/include/strings.h /usr/include/time.h /usr/include/unistd.h
main.o: /archive/Dev/Projects/var/GOTS/include/debug.h
