#
# Template makefile
# EXE = name of final executable
# SRC = list of source files
# LOCAL_CFLAGS = list of private Compiler flags
# LOCAL_LIBES = list of libraries to use ( in -lxxx format )
# DIP_LIBS = list of library pathnames used by the application
#

CC = gcc
LD = gcc
EXE = GOTSpooler
SRC = main.c lputils.c init.c papermount.c
LOCAL_CFLAGS = 

LOCAL_LIBES = -lshm \
			  -lcfg \
			  -ldebug

LOCAL_LIBESD = -lshm.d \
			   -lcfg.d \
			   -ldebug.d

DIP_LIBS =  ${PROJECT_HOME}/lib/libshm.a \
		    ${PROJECT_HOME}/lib/libcfg.a

include	${PROJECT_HOME}/Commons/makefiles/makefile.std
# DO NOT DELETE THIS LINE -- make depend depends on it.
