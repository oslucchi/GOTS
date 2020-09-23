#
# Template makefile for modules with many targets
# TARGETS = names of sub-modules (1 direcory for each modules)
# Ex:
# TARGETS = spv_stat stat_viewer s_collector
# Directory containing libraries first
#

TARGETS = libcfg libconnection libdebug libshm libstrutil

SHELL = /bin/sh


debug::
	for i in $(TARGETS) ; do cd $$i ; echo $$i ; make debug ; cd .. ; done

plain::
	for i in $(TARGETS) ; do cd $$i ; echo $$i; make all ; cd .. ; done

all::
	for i in $(TARGETS) ; do cd $$i ; echo $$i ; make install ; cd .. ; done
	for i in $(TARGETS) ; do cd $$i ; echo $$i ; make debug ; cd .. ; done

