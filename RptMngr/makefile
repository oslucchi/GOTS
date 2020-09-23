#
# Template makefile
# EXE = name of final executable
# SRC = list of source files
# LOCAL_CFLAGS = list of private Compiler flags
# LOCAL_LIBES = list of libraries to use ( in -lxxx format )
# DIP_LIBS = list of library pathnames used by the application
#

LD = gcc
EXE = GOTRptMng
SRC = hndlcli.c main.c sockop.c  

LOCAL_CFLAGS = -g 

LOCAL_LIBES = -lshm \
			  -lcfg \
			  -lconn \
			  -lstrutil \
			  -ldebug

LOCAL_LIBESD = -lshm.d \
			   -lcfg.d \
			   -lconn.d \
			   -lstrutil.d \
			   -ldebug.d

DIP_LIBS =  ${PROJECT_HOME}/lib/libshm.a \
		    ${PROJECT_HOME}/lib/libcfg.a

include	${PROJECT_HOME}/Commons/makefiles/makefile.std
# DO NOT DELETE THIS LINE -- make depend depends on it.
