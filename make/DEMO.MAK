#----------------------------------------------------------------
# makefile
#----------------------------------------------------------------

p =    ..\port
o =    ..\obj
b =    ..
r =    ..\resource
jpeg = ..\jpeg

COMMON = ..\lib

CC = icc /c /Gm+ /I$(COMMON) /I$(jpeg) /Kb /Gs- /O- /Sm /Ss /Q /Xi- /Ti /Ge+ /W3 /DDEMOVERSION
LNK = LINK386 /ALIGN:4 /NOLOGO /PMTYPE:PM
#-----------------------------------------------------------------
# File Lists
#-----------------------------------------------------------------

LIST1 = $(o)\drwmain.obj $(o)\dlg_hlp.obj $(o)\dlg_btn.obj  \
        $(o)\dlg_cir.obj $(o)\dlg_clr.obj $(o)\dlg_file.obj \
        $(o)\dlg_fnt.obj $(o)\drwgrp.obj   $(o)\drwpic.obj    \
        $(o)\drwfount.obj $(o)\drfrm24.obj

LIST2 = $(o)\drwimg.obj $(o)\dlg_lin.obj $(o)\drwprn.obj   \
        $(o)\dlg_sel.obj $(o)\dlg_sqr.obj $(o)\dlg_txt.obj   \
        $(o)\drwcbar.obj $(o)\drwtrian.obj $(o)\drwcreat.obj \
        $(o)\drwtxt.obj $(o)\drwcanv.obj

LIST3 = $(o)\drwimg2.obj $(o)\drwlayer.obj $(o)\drwutl.obj   \
        $(o)\drwtxtin.obj $(o)\drwmeta.obj $(o)\drwtbar.obj  \
        $(o)\drwarr.obj

LIST4 = $(o)\drwmenu.obj $(o)\drwsplin.obj $(o)\drwsplsh.obj \
        $(o)\drwform.obj $(o)\drwmclb.obj  $(o)\drwfile.obj  \
        $(o)\drwjpg.obj



#----rules--------------------------------

all: $(b)\drawit.exe $(b)\drawit.hlp

{$(p)\}.c{$(o)\}.obj:
   $(CC) -Fo$@ $<

$(o)\drawit.res : ..\resource\drawit.rc
  rc -r ..\resource\drawit.rc

##$(b)\drawit.hlp : ..\help\drawit.ipf
##  ipfc  ..\help\drawit.ipf
##  copy  ..\help\drawit.hlp ..



$(b)\drawit.exe: $(LIST1) \
                 $(LIST2) \
                 $(LIST3) \
                 $(LIST4) \
                 $(r)\drawit.res
   $(LNK) @<< <<drawit.def
$(LIST1) +
$(LIST2) +
$(LIST3) +
$(LIST4),
$@,
drawit.map,
..\lib\drwgbm.lib ..\jpeg\libjpeg.lib,
drawit.def
<<NOKEEP
NAME            DRAWIT WINDOWAPI
DESCRIPTION     'DRAWIT for OS/2'
EXETYPE         OS2
PROTMODE
HEAPSIZE        18092
STACKSIZE       55000
<<NOKEEP
   rc ..\resource\drawit.res ..\drawit.exe

$(o)\drwmain.obj   : $(p)\drwmain.c   $(p)\dlg.h $(p)\resource.h
$(o)\dlg_hlp.obj  :  $(p)\dlg_hlp.c  $(p)\dlg.h
$(o)\dlg_btn.obj  :  $(p)\dlg_btn.c  $(p)\dlg.h
$(o)\dlg_cir.obj  :  $(p)\dlg_cir.c  $(p)\dlg.h
$(o)\dlg_clr.obj  :  $(p)\dlg_clr.c  $(p)\dlg.h
$(o)\dlg_file.obj :  $(p)\dlg_file.c $(p)\dlg.h
$(o)\dlg_fnt.obj  :  $(p)\dlg_fnt.c  $(p)\dlg.h
$(o)\dlg_hlp.obj  :  $(p)\dlg_hlp.c  $(p)\dlg.h
$(o)\drwimg.obj   :  $(p)\drwimg.c  $(p)\dlg.h
$(o)\dlg_lin.obj  :  $(p)\dlg_lin.c  $(p)\dlg.h
$(o)\drwprn.obj   :  $(p)\drwprn.c   $(p)\dlg.h
$(o)\dlg_sel.obj  :  $(p)\dlg_sel.c  $(p)\dlg.h
$(o)\dlg_sqr.obj  :  $(p)\dlg_sqr.c  $(p)\dlg.h
$(o)\dlg_txt.obj  :  $(p)\dlg_txt.c  $(p)\dlg.h
$(o)\drwtxt.obj   :  $(p)\drwtxt.c   $(p)\dlg.h
$(o)\drwcbar.obj  :  $(p)\drwcbar.c  $(p)\dlg.h
$(o)\drwtrian.obj :  $(p)\drwtrian.c $(p)\dlg.h
$(o)\drwcreat.obj :  $(p)\drwcreat.c $(p)\dlg.h
$(o)\drwmenu.obj  :  $(p)\drwmenu.c  $(p)\dlg.h $(p)\resource.h
$(o)\drwlayer.obj :  $(p)\drwlayer.c $(p)\dlg.h
$(o)\drwutl.obj   :  $(p)\drwutl.c   $(p)\dlg.h  $(p)\drwgrp.h
$(o)\drwmeta.obj  :  $(p)\drwmeta.c  $(p)\dlg.h
$(o)\drwsplsh.obj :  $(p)\drwsplsh.c
$(o)\drwsplin.obj :  $(p)\drwsplin.c $(p)\dlg.h
$(o)\drwgrp.obj   :  $(p)\$(@B).c    $(p)\$(@B).h $(p)\dlg.h
$(o)\drwimg2.obj  :  $(p)\drwimg2.c  $(p)\dlg.h
$(o)\drwcanv.obj  :  $(p)\drwcanv.c  $(p)\dlg.h
$(o)\drwform.obj  :  $(p)\drwform.c  $(p)\dlg.h
$(o)\drwmclb.obj  :  $(p)\drwmclb.c  $(p)\dlg.h
$(o)\drwpic.obj   :  $(p)\drwpic.c   $(p)\dlg.h
$(o)\drfrm24.obj  :  $(p)\drfrm24.c   $(p)\dlg.h
$(o)\drwfile.obj  :  $(p)\drwfile.c  $(p)\dlg.h
$(o)\drwarr.obj   :  $(p)\drwarr.c  $(p)\dlg.h
$(o)\drwjpg.obj   :  $(p)\$(@B).c    $(p)\dlg.h

