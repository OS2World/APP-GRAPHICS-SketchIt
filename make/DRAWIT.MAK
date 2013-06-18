#----------------------------------------------------------------
# makefile
#----------------------------------------------------------------

p =    ..\port
o =    ..\obj
b =    ..
r =    ..\resource
jpeg = ..\jpeg

COMMON = ..\lib

CC = icc /c /Gm+ /I$(COMMON) /I$(jpeg) /Kb /Gs- /O- /Sm /Ss /Q /Xi- /Ti /Ge+ /W3
LNK = LINK386 /code /ALIGN:4 /NOLOGO /PMTYPE:PM
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
        $(o)\drwarr.obj   $(o)\drwbtext.obj

LIST4 = $(o)\drwmenu.obj $(o)\drwsplin.obj $(o)\drwsplsh.obj \
        $(o)\drwform.obj $(o)\drwmclb.obj  $(o)\drwfile.obj  \
        $(o)\drwjpg.obj



#----rules--------------------------------

all: $(b)\drawit.exe $(b)\drawit.hlp

{$(p)\}.cpp{$(o)\}.obj:
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

$(o)\drwmain.obj   : $(p)\drwmain.cpp   $(p)\dlg.h $(p)\resource.h
$(o)\dlg_hlp.obj  :  $(p)\dlg_hlp.cpp  $(p)\dlg.h
$(o)\dlg_btn.obj  :  $(p)\dlg_btn.cpp  $(p)\dlg.h
$(o)\dlg_cir.obj  :  $(p)\dlg_cir.cpp  $(p)\dlg.h
$(o)\dlg_clr.obj  :  $(p)\dlg_clr.cpp  $(p)\dlg.h
$(o)\dlg_file.obj :  $(p)\dlg_file.cpp $(p)\dlg.h
$(o)\dlg_fnt.obj  :  $(p)\dlg_fnt.cpp  $(p)\dlg.h
$(o)\dlg_hlp.obj  :  $(p)\dlg_hlp.cpp  $(p)\dlg.h
$(o)\drwimg.obj   :  $(p)\drwimg.cpp  $(p)\dlg.h
$(o)\dlg_lin.obj  :  $(p)\dlg_lin.cpp  $(p)\dlg.h
$(o)\drwprn.obj   :  $(p)\drwprn.cpp   $(p)\dlg.h
$(o)\dlg_sel.obj  :  $(p)\dlg_sel.cpp  $(p)\dlg.h
$(o)\dlg_sqr.obj  :  $(p)\dlg_sqr.cpp  $(p)\dlg.h
$(o)\dlg_txt.obj  :  $(p)\dlg_txt.cpp  $(p)\dlg.h
$(o)\drwtxt.obj   :  $(p)\drwtxt.cpp   $(p)\dlg.h
$(o)\drwcbar.obj  :  $(p)\drwcbar.cpp  $(p)\dlg.h
$(o)\drwtrian.obj :  $(p)\drwtrian.cpp $(p)\dlg.h
$(o)\drwcreat.obj :  $(p)\drwcreat.cpp $(p)\dlg.h
$(o)\drwmenu.obj  :  $(p)\drwmenu.cpp  $(p)\dlg.h $(p)\resource.h
$(o)\drwlayer.obj :  $(p)\drwlayer.cpp $(p)\dlg.h
$(o)\drwutl.obj   :  $(p)\drwutl.cpp   $(p)\dlg.h  $(p)\drwgrp.h
$(o)\drwmeta.obj  :  $(p)\drwmeta.cpp  $(p)\dlg.h
$(o)\drwsplsh.obj :  $(p)\drwsplsh.cpp
$(o)\drwsplin.obj :  $(p)\drwsplin.cpp $(p)\dlg.h
$(o)\drwgrp.obj   :  $(p)\$(@B).cpp    $(p)\$(@B).h $(p)\dlg.h
$(o)\drwimg2.obj  :  $(p)\drwimg2.cpp  $(p)\dlg.h
$(o)\drwcanv.obj  :  $(p)\drwcanv.cpp  $(p)\dlg.h
$(o)\drwform.obj  :  $(p)\drwform.cpp  $(p)\dlg.h
$(o)\drwmclb.obj  :  $(p)\drwmclb.cpp  $(p)\dlg.h
$(o)\drwpic.obj   :  $(p)\drwpic.cpp   $(p)\dlg.h
$(o)\drfrm24.obj  :  $(p)\drfrm24.cpp   $(p)\dlg.h
$(o)\drwfile.obj  :  $(p)\drwfile.cpp  $(p)\dlg.h
$(o)\drwarr.obj   :  $(p)\drwarr.cpp  $(p)\dlg.h
$(o)\drwjpg.obj   :  $(p)\$(@B).cpp    $(p)\dlg.h
$(o)\drwbtext.obj :  $(p)\$(@B).cpp    $(p)\dlg.h
