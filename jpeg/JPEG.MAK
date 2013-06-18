# Makefile for Independent JPEG Group's software
# This makefile is for IBM CSET++.
#
# Ported to OS/2 2.x by Jasper de Keijzer, The Netherlands 1994
#
# Read installation instructions before saying "nmake" !!

# You may need to adjust these compiler options:
## COMPILER ##################################################################
#
# /c        Perform compile only.
# /D        Define preprocessor macros.
# /Ge[+|-]  Create [Exe|DLL].
# /Gm     * Link with multithreaded libraries.
# /Gs-    * Generate stack probes.
# /Kb     * Produce basic diagnostic messages.
# /O[-]     [No] optimize code.
# /Sm     * Allow migration extensions.
# /Sp1      0-byte alignment
# /Ss       Allow // to indicate comments.
# /Q        Do not display logo.
# /Ti       Prepare for debugging.
# /W3       Warning level 3 (highest).
# /Xi-      Search paths specified by the INCLUDE variable.
# /P+       Generate precompiler output
#

base    = f:\drw29
INCL    = $(base)\port
o       = $(base)\jpeg


CSET = /c /Gm /Gs- /O- /Sm /Ss /Q /Xi- /O- /I$(INCL)
##CFLAGS = /c /Gm /Gs- /Wgrp /O- /Sm /Ss /Q /Xi- /ti /O-
##CC = icc $(CFLAGS) /Ge+ /W3
CC = icc $(CSET) /Ge+ /W3
LNK = ILINK /CODE /FREE /ALIGN:4 /STACK:14096 /NOLOGO /PMTYPE:VIO
LNKWIN = ILINK /CODE /FREE /ALIGN:4 /STACK:14096 /NOLOGO /PMTYPE:PM
LNK386 = LINK386 /ALIGN:4 /NOLOGO /PMTYPE:PM

SYSDEPMEM= jmemansi.obj

LIBSRC= jcapi.c    jccoefct.c jccolor.c jcdctmgr.c jchuff.c jcmainct.c \
        jcmarker.c jcmaster.c jcomapi.c jcparam.c jcprepct.c jcsample.c \
        jdapi.c jdatasrc.c jdatadst.c jdcoefct.c jdcolor.c jddctmgr.c   \
        jdhuff.c jdmainct.c jdmarker.c jdmaster.c jdpostct.c jdsample.c \
        jerror.c jutils.c jfdctfst.c jfdctflt.c jfdctint.c jidctfst.c   \
        jidctflt.c jidctint.c jidctred.c jquant1.c jquant2.c jdmerge.c  \
        jmemmgr.c  jmemname.c jmemnobs.c

LIB1= $(o)\jcapi.obj   $(o)\jccoefct.obj $(o)\jccolor.obj  $(o)\jcdctmgr.obj \
      $(o)\jchuff.obj  $(o)\jcmainct.obj $(o)\jcmarker.obj $(o)\jcmaster.obj \
      $(o)\jcomapi.obj $(o)\jcparam.obj  $(o)\jcprepct.obj $(o)\jcsample.obj \
      $(o)\jdapi.obj   $(o)\jdatasrc.obj $(o)\jdatadst.obj $(o)\jdcoefct.obj \
      $(o)\jdcolor.obj $(o)\jddctmgr.obj $(o)\jdhuff.obj   $(o)\jdmainct.obj

LIB2= $(o)\jdmarker.obj $(o)\jdmaster.obj $(o)\jdpostct.obj $(o)\jdsample.obj \
      $(o)\jerror.obj   $(o)\jutils.obj   $(o)\jfdctfst.obj $(o)\jfdctflt.obj \
      $(o)\jfdctint.obj $(o)\jidctfst.obj $(o)\jidctflt.obj $(o)\jidctint.obj \
      $(o)\jidctred.obj $(o)\jquant1.obj  $(o)\jquant2.obj  $(o)\jdmerge.obj  \
      $(o)\jmemmgr.obj  $(o)\jmemname.obj $(o)\jmemnobs.obj \
      $(o)\jpmerror.obj

MYOBJECTS= drwjptmp.obj wrrle.obj drwjpg.obj rdcolmap.obj
#############################################################
# rules
#############################################################

.c.obj:
   $(CC) $<

all: libjpeg.lib

MYOBJECTS= drwjptmp.obj wrrle.obj drwjpg.obj rdcolmap.obj


libjpeg.lib: $(LIB1) $(LIB2)
    lib @<<
libjpeg.lib /NOLOGO + jcapi.obj     + jcparam.obj   &
+ $(o)\jdatadst.obj  + $(o)\jcmaster.obj  &
+ $(o)\jcmarker.obj  + $(o)\jcmainct.obj  &
+ $(o)\jcprepct.obj  + $(o)\jccoefct.obj  &
+ $(o)\jccolor.obj   + $(o)\jcsample.obj  &
+ $(o)\jchuff.obj    + $(o)\jcdctmgr.obj  &
+ $(o)\jfdctfst.obj  + $(o)\jfdctflt.obj  &
+ $(o)\jfdctint.obj  + $(o)\jdapi.obj     &
+ $(o)\jdatasrc.obj  + $(o)\jdmaster.obj  &
+ $(o)\jdmarker.obj  + $(o)\jdmainct.obj  &
+ $(o)\jdcoefct.obj  + $(o)\jdpostct.obj  &
+ $(o)\jddctmgr.obj  + $(o)\jidctfst.obj  &
+ $(o)\jidctflt.obj  + $(o)\jidctint.obj  &
+ $(o)\jidctred.obj  + $(o)\jdhuff.obj    &
+ $(o)\jdsample.obj  + $(o)\jdcolor.obj   &
+ $(o)\jquant1.obj   + $(o)\jquant2.obj   &
+ $(o)\jdmerge.obj   + $(o)\jcomapi.obj   &
+ $(o)\jutils.obj    + $(o)\jPMerror.obj  &
+ $(o)\jmemmgr.obj   + $(o)\jmemnobs.obj;
<<


myjpeg.exe: $(MYOBJECTS)
   $(LNKWIN) $(MYOBJECTS) /out:$@ libjpeg.lib
   rc drwjpg.res myjpeg.exe

drwjpg.res : drwjpg.rc
   rc -r drwjpg.rc

test.exe: $(MYOBJECTS)
   $(LNK386) @<< <<drawit.def
$(MYOBJECTS),
$@,
drawit.map,
libjpeg.lib DDE4MBS,
drawit.def
<<NOKEEP
NAME            TEST WINDOWAPI
DESCRIPTION     'DRAWIT for OS/2'
EXETYPE         OS2
PROTMODE
HEAPSIZE        18092
STACKSIZE       55000
<<NOKEEP
   rc drwjpg.res test.exe


jconfig.h: jconfig.doc
	echo You must prepare a system-dependent jconfig.h file.
	echo Please read the installation directions in install.doc.
	exit 1
