# Copyright (c) 1997, 2000 Cygnus Support
#
# The authors hereby grant permission to use, copy, modify, distribute,
# and license this software and its documentation for any purpose, provided
# that existing copyright notices are retained in all copies and that this
# notice is included verbatim in any distributions. No written agreement,
# license, or royalty fee is required for any of the authorized uses.
# Modifications to this software may be copyrighted by their authors
# and need not follow the licensing terms described here, provided that
# the new terms are clearly indicated on the first page of each file where
# they apply.

DESTDIR =
VPATH = ../../.././libgloss/i386
srcdir = ../../.././libgloss/i386
objdir = .
srcroot = $(srcdir)/../..
objroot = $(objdir)/../..

prefix = /Users/evil/dev/c/YiYiYa/eggs/libnewlib/lib
exec_prefix = ${prefix}

host_alias = i386-elf
target_alias = i386-elf
program_transform_name = s&^&i386-elf-&

bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
tooldir = $(exec_prefix)/$(target_alias)

# Multilib support variables.
# TOP is used instead of MULTI{BUILD,SRC}TOP.
MULTIDIRS = 
MULTISUBDIR = 
MULTIDO = $(MAKE)
MULTICLEAN = $(MAKE)

INSTALL = /opt/local/bin//ginstall -c
INSTALL_PROGRAM = /opt/local/bin//ginstall -c
INSTALL_DATA = /opt/local/bin//ginstall -c -m 644

SHELL =	/bin/sh

mkinstalldirs = $(SHELL) $(srcroot)/mkinstalldirs

CC = i386-elf-gcc -B/Users/evil/dev/c/YiYiYa/eggs/libnewlib/i386-elf/newlib/ -isystem /Users/evil/dev/c/YiYiYa/eggs/libnewlib/i386-elf/newlib/targ-include -isystem /Users/evil/dev/c/YiYiYa/eggs/libnewlib/newlib/libc/include -B/Users/evil/dev/c/YiYiYa/eggs/libnewlib/i386-elf/libgloss/i386 -L/Users/evil/dev/c/YiYiYa/eggs/libnewlib/i386-elf/libgloss/libnosys -L/Users/evil/dev/c/YiYiYa/eggs/libnewlib/libgloss/i386    -nostdinc  -isystem /opt/local/lib/gcc/i386-elf/9.2.0/include -isystem /opt/local/lib/gcc/i386-elf/9.2.0/include-fixed

#AS = @AS@
AS = `if [ -f ${objroot}/../gas/as.new ] ; \
	then echo ${objroot}/../gas/as.new ; \
	else echo as ; fi`

AR = i386-elf-ar

#LD = i386-elf-ld
LD = `if [ -f ${objroot}/../ld/ld.new ] ; \
	then echo ${objroot}/../ld/ld.new ; \
	else echo ld ; fi`

RANLIB = i386-elf-ranlib

OBJDUMP = `if [ -f ${objroot}/../binutils/objdump ] ; \
	then echo ${objroot}/../binutils/objdump ; \
	else t='$(program_transform_name)'; echo objdump | sed -e $$t ; fi`
OBJCOPY = `if [ -f ${objroot}/../binutils/objcopy ] ; \
	then echo ${objroot}/../binutils/objcopy ; \
	else t='$(program_transform_name)'; echo objcopy | sed -e $$t ; fi`

CRT0 = cygmon-crt0.o
CYGMON_OBJS = cygmon-salib.o cygmon-gmon.o

CFLAGS = -g

GCC_LDFLAGS = `if [ -d ${objroot}/../gcc ] ; \
	then echo -L${objroot}/../gcc ; fi`

SCRIPTS = cygmon
BSP = libcygmon.a

# Host specific makefile fragment comes in here.
NEWLIB_CFLAGS = `if [ -d ${objroot}/newlib ]; then echo -I${objroot}/newlib/targ-include -I${srcroot}/newlib/libc/include; fi`
NEWLIB_LDFLAGS = `if [ -d ${objroot}/newlib ]; then echo -B${objroot}/newlib/ -L${objroot}/newlib/; fi`

INCLUDES = -I. -I$(srcdir)/.. -I$(objdir)/..
# Note that when building the library, ${MULTILIB} is not the way multilib
# options are passed; they're passed in $(CFLAGS).
CFLAGS_FOR_TARGET = -O2 -g ${MULTILIB} ${INCLUDES} ${NEWLIB_CFLAGS}
LDFLAGS_FOR_TARGET = ${MULTILIB} ${NEWLIB_LDFLAGS}
AR_FLAGS = rc

.c.o:
	$(CC) $(CFLAGS_FOR_TARGET) -O2 $(INCLUDES) -c $(CFLAGS) $<

.C.o:
	$(CC) $(CFLAGS_FOR_TARGET) -O2 $(INCLUDES) -c $(CFLAGS) $<
.s.o:
	$(AS) $(ASFLAGS_FOR_TARGET) $(INCLUDES) $(ASFLAGS) -o $*.o $<

#
# GCC knows to run the preprocessor on .S files before it assembles them.
#
.S.o:
	$(CC) $(CFLAGS_FOR_TARGET) $(INCLUDES) $(CFLAGS) -c $<

#
# this is a bogus target that'll produce an assembler from the
# C source with the right compiler options. this is so we can
# track down code generation or debug symbol bugs.
#
.c.s:
	$(CC) $(CFLAGS_FOR_TARGET) -S $(INCLUDES) $(CFLAGS) $<

# We don't build docs in subdirs, so stub out the rules.
.PHONY: doc docs dvi html install-html info install-info clean-info pdf install-pdf

all: ${CRT0} ${BSP}

#
# here's where we build the board support packages for each target
#
libcygmon.a: $(CYGMON_OBJS)
	${AR} ${ARFLAGS} $@ $(CYGMON_OBJS)
	${RANLIB} $@

cygmon-salib.o: ${srcdir}/cygmon-salib.c
	$(CC) -c $(CFLAGS) $(I386_CPPFLAGS) $(<) -o $@

cygmon-crt0.o: ${srcdir}/cygmon-crt0.S
	$(CC) -c $(CFLAGS) $(I386_CPPFLAGS) $(<) -o $@

doc:	

clean mostlyclean:
	rm -f a.out core *.i *~ *.o *-test *.srec *.dis *.map *.x

distclean maintainer-clean realclean: clean
	rm -f Makefile a.out

.PHONY: install info install-info clean-info
install:
	${mkinstalldirs} ${DESTDIR}${tooldir}/lib${MULTISUBDIR}
	$(INSTALL_PROGRAM) $(CRT0) $(DESTDIR)$(tooldir)/lib${MULTISUBDIR}/$(CRT0)
	@for bsp in ${BSP}; do\
	 $(INSTALL_PROGRAM) $${bsp} $(DESTDIR)$(tooldir)/lib${MULTISUBDIR}; \
	done
	@for script in ${SCRIPTS}; do\
	 $(INSTALL_DATA) ${srcdir}/$${script}.ld $(DESTDIR)$(tooldir)/lib${MULTISUBDIR}/$${script}.ld; \
	done

info:
install-info:
clean-info:

test.o:	${srcdir}/test.c

# these are for the BSPs
${CRT0}: cygmon-crt0.S

Makefile: Makefile.in ../config.status
	cd .. && $(SHELL) config.status

PWD_COMMAND=$${PWDCMD-pwd}

# FIXME: There should be an @-sign in front of the `if'.
# Leave out until this is tested a bit more.
multi-do:
	if [ -z "$(MULTIDIRS)" ]; then \
	  true; \
	else \
	  rootpre=`${PWD_COMMAND}`/; export rootpre; \
	  srcrootpre=`cd $(srcdir); ${PWD_COMMAND}`/; export srcrootpre; \
	  lib=`echo "$${rootpre}" | sed -e 's,^.*/\([^/][^/]*\)/$$,\1,'`; \
	  compiler="$(CC)"; \
	  for i in `$${compiler} --print-multi-lib 2>/dev/null`; do \
	    dir=`echo $$i | sed -e 's/;.*$$//'`; \
	    if [ "$${dir}" = "." ]; then \
	      true; \
	    else \
	      if [ -d ../$${dir}/$${lib} ]; then \
		flags=`echo $$i | sed -e 's/^[^;]*;//' -e 's/@/ -/g'`; \
		if (cd ../$${dir}/$${lib}; $(MAKE) $(FLAGS_TO_PASS) \
				CFLAGS="$(CFLAGS) $${flags}" \
				CCASFLAGS="$(CCASFLAGS) $${flags}" \
				FCFLAGS="$(FCFLAGS) $${flags}" \
				FFLAGS="$(FFLAGS) $${flags}" \
				ADAFLAGS="$(ADAFLAGS) $${flags}" \
				prefix="$(prefix)" \
				exec_prefix="$(exec_prefix)" \
				GCJFLAGS="$(GCJFLAGS) $${flags}" \
				GOCFLAGS="$(GOCFLAGS) $${flags}" \
				CXXFLAGS="$(CXXFLAGS) $${flags}" \
				LIBCFLAGS="$(LIBCFLAGS) $${flags}" \
				LIBCXXFLAGS="$(LIBCXXFLAGS) $${flags}" \
				LDFLAGS="$(LDFLAGS) $${flags}" \
				MULTIFLAGS="$${flags}" \
				DESTDIR="$(DESTDIR)" \
				INSTALL="$(INSTALL)" \
				INSTALL_DATA="$(INSTALL_DATA)" \
				INSTALL_PROGRAM="$(INSTALL_PROGRAM)" \
				INSTALL_SCRIPT="$(INSTALL_SCRIPT)" \
				$(DO)); then \
		  true; \
		else \
		  exit 1; \
		fi; \
	      else true; \
	      fi; \
	    fi; \
	  done; \
	fi

# FIXME: There should be an @-sign in front of the `if'.
# Leave out until this is tested a bit more.
multi-clean:
	if [ -z "$(MULTIDIRS)" ]; then \
	  true; \
	else \
	  lib=`${PWD_COMMAND} | sed -e 's,^.*/\([^/][^/]*\)$$,\1,'`; \
	  for dir in : $(MULTIDIRS); do \
	    test $$dir != : || continue; \
	    if [ -f ../$${dir}/$${lib}/i386/Makefile ]; then \
	      if (cd ../$${dir}/$${lib}; $(MAKE) $(FLAGS_TO_PASS) $(DO)); \
	      then true; \
	      else exit 1; \
	      fi; \
	    else true; \
	    fi; \
	  done; \
	fi
