:userdoc.
.*--------------------------------------------------------------*\
.*  Main Help menu
.*      res = PANEL_HELP
.*--------------------------------------------------------------*/
:h1 res=2910 name=PANEL_HELP.Getting Started.
:i1. Getting Started
:p.This drawing tool is written for OS/2 2.x or higher.
Here we will give an overview of what is possible with Drawit.
What objects are, how to select, copy, delete and move objects.
.br
:p.Overview:
.br
All objects can be moved and sized when they are
:link reftype=hd res=3400.selected:elink..
.br
Deleted when selected with the <Delete> key on the keyboard.
.br
Copied when moving is done while holding down the <Ctrl> key on the keyboard.
.br
Stretching: when the mouse cursor is moved over the handles of the selected 
object the cursor changes in a double arrow. If this is the case, 
the object can be stretched by holding down the left mouse button and drag
the mouse. If the mouse button is released the object will appear in it's
new size.
.br
Filled objects can have 10 different filling patterns, also lines can be
created in 8 different types. See the attributes menuitem.
.br
Example of different filling patterns.
:artwork align=left name='..\help\filling.bmp'.
Drawing is done by clicking with the mouse on the drawing area and start moving
while holding down the left mouse button. When releasing the button the object
will be drawn in the color selected in the standard palette at the bottom of
the screen.
.br
:link reftype=hd res=3200.Colors.:elink. can be changed by clicking on the appropriate color in the color palette
at the bottom of the screen. If the color does not meet the requirements, double
click on the color in the palette. This action will bring up a dialog which
ables you to remix the color. The color of an object can also be changed via the 
OS/2 color palette.
.br
Properties can be changed by double click on the object.
.br
Ellipse and Square objects will be drawn as circle or as symetric square when
the <Shift> key is pressed while drawing is done.
.br
.br
:hp2.Multiselection:ehp2. If the dragging is done in the drawing area (Moving the
mouse while holding down the left button) :hp8.DRAWIT:ehp8. draws a selection square
which disappears when no object are laying in the square. 
.br
Attributes of one ore more drawn objects can be changed when there is a
selection. The following actions can be done on selected objects.
.br
:lm margin=5.
:hp2.Changing the font:ehp2. off all selected text objects, by dragging 
a font from the OS/2 font palette on the drawing area.
.br
:hp2.Changing the colors:ehp2. of the selected
objects by dragging a color from the OS/2 color palette over the selection area.
.br
:hp2.Copying:ehp2. the selected objects by holding down the <Ctrl> key while
moving the selection square with the mouse.
.br
:hp2.Moving:ehp2. by draging the selection square to the position you want.
.br
:hp2.Deleting:ehp2. by pressing the <Delete> key on the keyboard.
.br
:hp2.Filling pattern:ehp2. can be changed via the popupmenu or the options
menu etc.
:lm margin=1.
.br
:hp2.Popupmenu's :ehp2.
.br
The program has two popupmenu's. Click with the right mousebutton on the drawing
area to bring up a popupmenu. When a bitmap image is selected the program
shows the imagemenu as popupmenu. When nothing or an other object is selected 
the program will show a subset of the optionmenu as popupmenu. The popupmenu's
enables you to quickly change the attributes of a multi or single selection. Or
you can set the program attributes for the objects to be drawn.
.br
:artwork align=left name='..\resource\select.bmp'.
.br
The selectbutton switches the application into select mode. The <ESC> key 
can also be used for this purpose. After having this button press you
can start making :link reftype=hd res=3400.selections:elink.
.br
:artwork align=left name='..\resource\text.bmp'.
With this button you start the text input
of the program. The cursor start blinking in the upper right corner. To
position the cursor and start a new text object just click with the mouse
at the place where you would like to start the text. The cursor will be
replaced at that point. You can type in any character from the keyboard and
use the backspace key for deletion of mistakes. To stop editting or text
input use the <ESC> key or the upper mode button with the selection arrow.
If the current font in the textobject does not meet your requirements you
simply drag another font over the textobject. The textobject will imidiately
show itself with the new font. The size is not taken from the OS/2 font
palette. When you move the mouse cursor above the text object and you click
once with the left mouse button you select the text. On each corner there will
be little squares also named handles. If you now click on the text and keep
the left mouse button pressed and start moving, it (DRAGGING) the handles will
move with the cursor. When the mouse button it released the text will be
redrawn at the new place. When you double click on the text a dialog comes
up which enables you to change the text properties.
.br
:artwork align=left name='..\resource\circle.bmp'.
With this button you can start the creation of ellipses. Click with
mouse in the drawing area and move while holding down the button. 
When the mouse button is released the ellipse is drawn in its final state.
If the <Shift> key is pressed while the ellipse is drawn, the shape will
stay exacly circular. 
Also have a look in the objects menu there is an option for partial arcs!
The radius of the partial arc is defined by the amound of moving in
the X-direction while drawing is done. While the part of the pie will
grow when the mouse moves in the Y-direction.
.br
:artwork align=left name='..\resource\square.bmp'.
With this button you can start the creation of  squares
The filling pattern can be set in the options menu. There are 16
different patterns available. As with the ellipses you can hold down the 
<Shift> key, while drawing, to keep the apsect ratio exacly square.
.br
:artwork align=left name='..\resource\line.bmp'.
With this button you can start the creation of straight lines. A line
starts at the point you click with the mouse. By holding down the mouse
button while moving the mouse the line increases in size. Releasing the
mouse sets the end point of the line. When the program is in select mode
(by pressing the select button or the <ESC> key on the keyboard) the line
position and size can be changed by selecting the line. Moving is done
by clicking in the square area occupied by the line, and moving the mouse
while holding down the left mouse button. Size and direction can be 
changed by clicking on one of the available handles and moving the mouse
while holding down the button. Important, in the option menu you can specify
16 different line types!!
.br
Also have a look in the objects menu, there is an SPECIAL LINE option which
ables you to creat a collection of lines in one go.
.br
1. Select the special lines option in the object menu.
.br
2. Click with the mouse in the drawing area and hold down the mouse button.
.br
3. Move the mouse in a circular shape for instance.
.br
4. Look!!
.br
:artwork align=left name='..\resource\trian.bmp'.
This button ables you to create regular polypoints. It will bring up a small
dialog window which appears in the upper rightcorner of the application. With
this dialog you can define the number of points in the figure. This figure
is of the same class as the polyline. Besides the normal stretch functionality
it is possible to change the shape. When the number of points is set to its
max value then the shape gets circular. This can be used to create
:link reftype=hd res=5100.balloon text:elink.
.br
:artwork align=left name='..\resource\fill.bmp'.
This button starts for you the OS/2 color palette. Now you can change
the colors of all objects (except images), just by dragging over the
selected color. Only the objects in the current :link reftype=hd res=3800.layer:elink.
are affected.
.br
:artwork align=left name='..\resource\font.bmp'.
Starts up the OS/2 font dialog and enables you to change the
font of drawit by draging the selected font over the drawing area.
Also fonts can be changed of text objects by dragging the the
selected font over an selected font.
:artwork align=left name='..\resource\layer.bmp'.
Starts up a dialog which shows you which objects are on the
current layer. This program supports 10 different layer. Layer
1 is the bottom layer while layer 10 is the top layer. Only objects
in the active or current layer can be selected with the mouse.
To avoid you get lost, you can run in this dialog through the layers
and see which objects are on which particular layer. Double click
on a circle, square etc to change the layer of the particular object.
.br
:artwork align=left name='..\resource\splhit.bmp'.
When a polyline,polyfillet, regular polypoint, regular polystar or a circle
is selected this button can be used
to show the handles on the linejoins. When the above mentioned object
is in this state the handles can be used to change the form.
.br
1. Select the above named object.
.br
2. Press on the above mentioned button.
.br
3. Click in one of the open selection boxes and hold down the mousebutton.
.br
4. Move the mouse.
.br
5. Release the mouse button.
.br
6. And look!
.br
:artwork align=left name='..\resource\objch.bmp'.
.br
Use this button to put one object in front of another object.
Example: 
.br
1. Draw two squares (preferibly filled with different colors). Square 1 overlayes
square 2.
.br
2. Press <ESC> key or the select button.
.br
3. Press the <Shift> key and click with the mouse on square 1 and after that on square 2.
Release the <Shift> key. Both squares should show their selection boxes.
.br
4. Click on the button.
.br
5. Result, square 2 overlaps square 1.
.br
:artwork align=left name='..\resource\zoom.bmp'.
.br
This button allows you to see the form in two zoom modes. By default
the form has excact the size as in the real world. When this button
is pressed it toggles between the default and half the size.
.br
:artwork align=left name='..\resource\rotate.bmp'.
.br
When a object is selected this button can be used to
:link reftype=hd res=4800.rotate:elink. the object.
.*--------------------------------------------------------------*\
.*  Help menu About command help panel
.*      res = PANEL_HELPABOUT
.*--------------------------------------------------------------*/
:h1 res=2960 name=PANEL_HELPABOUT.About.
:i1. About
:hp2.DrawIt for OS/2 Version 2.4C :ehp2. [10 March 1997 ].
:artwork align=left name='..\resource\drlogo.bmp'.
.br
:nt. :hp8.The programmer of this program shall not be liable for any damages
arising out of your use of this program.:ehp8. :ent.
.br
:hp2.!! READ !!  New features and changes !! READ !!:ehp2.
:sl compact.
:li.The internal fileformat (.JSP) is changed!! So drawings made
with version 2.3 cannot be loaded!. Export them as bitmap or metafile
to use them in this version.
:li.All objects can be rotated with a precision of 1 degree.
:li.Groups can be sized!
:li.Gradient fill can be suppressed during drawing to speed up the development
of the drawing.
:li.Variable gridsize and display interval.
:li.Zooming.
:li.Form size is selectable.
:li.Copying while stretching or rotating an object is possible.
:li.Finding a color in a bitmap with the find color menu option.
:li.Splines and poly lines show their starting point. When you end your
line in this area or press enter during creation, the spline will 
be closed and the program is ready for the next spline.
:li.Right mouse button on the color palette change the outline color.
:li.Selection of single lines can only be done by clicking on the line
:li.Mouse pointers were not shown correct on VGA.
:li.The LETTER form had a wrong size.
:li.Saving a metafile in the JSP file was causing a program crash.
:li.When the selected images is saved to disk stretching etc is included.
Hence images can be enlarged.
:li.Clipboard problems are totally solved in this version!
:li.Polylines can be editted. You can add or delete point from a polygon
after it is drawn.
:li.Polypoint and regular polystar can be created with many more points!
:li.When the button was used to move one object in front of another the program
could crash when just one object was selected.
:li.Saving a selected bitmap was always saved in true color if no radio button
selection was changed.
:esl.
.br
:hp2.Known problems and limitations:ehp2.
:sl compact.
:li.On standard VGA the text object is incorrect inversed.
:li.It is not possible yet to rotate Groups.
:li.On Cirrus logic VGA cards gradient fill is not shown when running
the 1.00 driver for the CL-GD5446-PCI board. It is recommended to not
use this option in the program. Other Cirrus logic boards can also give
this problem although the exact type and software versions are not known.
:li.Some VGA graphics boards do not properly invert. So when text is
moved or size this can give akward look. Press Ctrl+R to refresh the
screen. This is only happening when the standard VGA driver is used.
:esl.
See :link reftype=hd res=2910.Getting started:elink. for an overview
of the program.
.br
If there are problems, comments or wishes please send a message 
to 101322.3665@compuserve.com
.br
.*--------------------------------------------------------------*\
.*  Help menu New command help panel
.*      res = PANEL_HELPHELPFORHELP
.*--------------------------------------------------------------*/
:h1 res=2920 name=PANEL_HELPHELPFORHELP.Images
:i1. Images
:p.Via the filemenu or via drag and drop, you can import bitmap images 
of the following formats:
.br
PCX - A file format of Zsoft Corp (c).
.br
GIF - A file format of Compuserve Inc.
.br
BMP - OS/2 1.x, 2.x and MS Windows 3.0.
.br
TIFF- A file format of HP and Microsoft.
.br
XBM - A X Window bitmap format.
.br
TARGA - Truevision.
.br
PIC - Lotus 123 chart file format. :link reftype=hd res=5000.Details:elink.
.br
In the menu are the actions which can be taken on the selected image.
Double clicking on the selected image will show the details of the
image.
Not shown but avalailable via the keyboard are the following options:
.br
:hp2.Brightness:ehp2. To increase the brightness of the selected bitmap 
image, press the b or B on the keyboard. It is not 
possible to decrease the brightness. Hence if the result is not what you 
expected you have to reload the original image from disk.
.br
:hp2.Inverting:ehp2. Via the I or i on the keyboard it is possible to invert the
selected image.
.br
:nt. Actions can only be taken on ONE image at a time, namely the selected image!
:ent.
Further we recommend to try out all image menu options to get an idea what is
possible and not.
.br
Menu options:
.br
:hp2.Details:ehp2.
.br
This menu option brings up the a dialog window showing the
some details of the selected image. Here you can change the layer
of the image.
.br
:hp2.Flip Image:ehp2.
.br
This option makes it possible to mirror the image over the x-axis or 
the y-axis.
.br
:hp2.Color Rotation:ehp2.
.br
This option can be activated via the image menu or the numeric plus '+' (up)  
and minus '-' (down) keys. Color rotation is nothing more than adding a 
value to all entries in the colorpalette of the selected image. To check
the results.
.br
1. Load a bitmap image.
.br
2. Select the image by clicking on the image with the left mouse button.
.br
3. Press the minus key on the numeric keyboard or use the image menu.
.br
4. Check the results.
.br
:hp2.Crop Selected Image:ehp2.
.br
Crop the selected image. If you need only a part of the loaded image
it is possible to cut out a part. Choose the option and click with
the mouse at the upper left point in the image e.g the point where you
want to start the cut out. Drag with the mouse, (moving while holding
down the left mouse button) over the image and now you see a square
appearing. This square shows the cut out at the moment you release the
mouse button. If you release to mouse button immediately by accident the
picture get a minimum size of 5 pixels. This avoids disappearing of the
image.
.br
:hp2.Convert to Black and White:ehp2.
.br
Converts the color image to a black anf white image.
.br
:hp2.Make circular:ehp2.
.br
Shows the image in a circular frame.
.br
:hp2.Make square:ehp2.
.br
Shows the image in a square frame which is the default.
.br
:hp2.Save selected Image as:ehp2.
.br
This option makes it possible to save the selected bitmap image in
one of the supported formats. The image will be saved with the same
size as it is on screen. Hence this gives the possibily to change
the original size of bitmaps.
.br
.br
:hp2.Filling patterns:ehp2.
When the image is selected the filling pattern of the image can be changed
in the attribute menu. If you give the image a haftoned filling pattern
and the color white you get a misty picture. Solid or gradient filling
is not supported.
.br
:nt.A circular shape is not reflected in the saved bitmap image.:ent.
.br
Hint try the popupmenu (which can be activated with the right mouse
button) when the image is selected.
.*--------------------------------------------------------------*\
.*  Adding text.
.*      res = ?
.*--------------------------------------------------------------*/
:h1 res=2970 name=2970.Text.
:i1.Text
:p.When a text is entered each time the mouse is clicked on
a new start position the program creates a new text object. Also
after pressing the enter key. These objects can be moved and resized
etc. Also you can double click on them to change it's attributes.
Changing the font of a text object can be done by dragging the appropriate
font from the OS/2 system font palette and drop it, by releasing the mouse,
above the text object. Only the font type is reflected not the size. 
Also when holding down the control key while moving a text object the
program will make a copy.
.br
Menu options:
.br
:hp2.Font:ehp2.
.br
This menu option brings the standard fontdialog of OS/2 up. 
and enables you to set the font used byt the program. This action can 
also be done via the font palette, by dragging the appropriate font on
the drawing area.
.br
:hp2.Selected text:ehp2.
.br
This option brings up the dialog which enables you to fine tune
the text. Giving it shadow or making outline text. This dialog
can also be activated via a double click with the left mouse button
on the selected text.
Here is an example how text can look with different shadow types, 
also outline text is shown in this example.
:artwork align=left name='..\help\fontx.bmp'.
.br
:link reftype=hd res=4300.CircularText:elink.
.br
:hp2.Char Box:ehp2.
.br
This option make the character box square and takes the highest
value. So when the X-value is bigger than the Y-value the X-value
will be used to make the font square. 
.br
:hp2.Flip:ehp2.
.br
This option makes it possible to flip the text horizontal or
vertical. Horizontal sets the text upside down.
.br
Text can be :link reftype=hd res=4800.rotated:elink as any other object.
:artwork align=left name='..\help\exmpl.bmp'.
.br
.*--------------------------------------------------------------*\
.*  Help on text dialog
.*      res = PANEL_HELPABOUT
.*--------------------------------------------------------------*/
:h1 res=3000 name=PANEL_TEXTOBJECT.Text object dialog
:p.Textobject dialog information
:p.1 Changing the color.
:p.Shade color, when this radio button is checked all selections 
in the color panel will only affect the shading color of the
characters, if the shade is greater than zero.
:p.2 Changing the :link reftype=hd res=3800.layer:elink. 
where upon the text is.
:p.With the spinbutton you can transfer the text to a different
layer within the drawing. Layer 1 is the bottom layer while layer
number 10 is the top layer. You can change the overall layer by
using the layer button.
:p.3 Changing the char box size.
With the spinbuttons at the topleft of the dialog you can change
the character size of the selected text. This is handy when two
different strings should have the exact same character size.
:p.4 Shear.
:p.This sliderbar gives you the posibility to change the shear of the
character in the string. Give it a try to see what the effect is.
.*--------------------------------------------------------------*\
.*  Indexhelp Changing the colors     
.*      res = 3200
.*--------------------------------------------------------------*/
:h1 res=3200 name=PANEL_CHANGINGCOLORS.Changing colors
:i1. Changing colors
:artwork align=left name='..\resource\fill.bmp'.
.br
At the bottom of the screen there are 32 different pre defined colors
which can be changed by double clicking with the mouse (left button)
on the color you want to change. This will bring up a dialog window which
enables you to remix the initial color,by changing primary colors red,green
and blue. The program does not work with a palette and it supports therefore 
true color. 
So changing the palette will not affect any drawing previously made.
This small color palette defines the initial color for objects when they
are drawn. Later the color of the objects can be changed by selecting
the object and then clicking on the appropriate color.
Also draging the required color from the system color palette on an object is
supported. The OS/2 system palette can be activated via the buttons with
the picture as is shown on top of this screen.
To change the background color you must activate the system colorpalette
and drag the appropriate color on the drawing area.
.br
In the lower left corner of the application is a box showing the
current color setting. It shows the :hp2.interior color:ehp2. setting and the 
:hp2.outline color:ehp2. setting.
.br
To change the colors of more than one object at the time, you have to make
a multiple :link reftype=hd res=3400.selection:elink.
.br
:ol compact.
:li. Make a multiple selection.
:li. Click on the a color in the program color palette at the bottom of the
 Also dropping a color from the OS/2 color palette is supported.
:li. All selected objects will show their new color and are unselected.
:eol.
Dependant on the option set in the options menu, the outline color or the
interior color will change.
.br
:nt.The outline color can also be changed by clicking with the right mouse
button on one of the colors in the color palette.:ent.
.br
:hp2.Find color:ehp2.
This option allows you find a color with the mouse.
.br
:ol compact.
:li.Select the :hp2.Find color:ehp2. menu option.
:li.Click with the mouse in the drawing area on the color you like to have
in the pallete.
:li.The program will make the color you clicked on the current filling color.
And the selected color in the palette will change to this color.
:eol.

.*--------------------------------------------------------------*\
.*  Indexhelp Changing the active layer     
.*      res = 3300
.*--------------------------------------------------------------*/
:h1 res=3300 name=PANEL_CHANGINGLAYER.Changing layer
:i1. Changing layer
:artwork align=left name='..\resource\layer.bmp'.
.br
Changing the layer can be done via the layer dialog which is activated
with the layer button, as shown above. The dialog shows you the
objects on the selected layer and thus enables you to find objects
back which are not on the current layer.
The program supports 10 different layers. The default layer, this is
the initial layer after the program is started, is 1. The first layer
is the far most layer. For example, to be sure that the half toned
circles are on top of the halftoned (half toned means transparant
colored) triangles we put the circles in layer 2 and the trangles 
in layer 1.
:artwork align=left name='..\help\layerx.bmp'.
.br
More about :link reftype=hd res=3800.layer:elink.
.*--------------------------------------------------------------*\
.*  Indexhelp Selecting objects
.*      res = 3400
.*--------------------------------------------------------------*/
:h1 res=3400 name=PANEL_MOVINGOBJECTS.Selecting objects
:i1. Selecting objects
:artwork align=left name='..\resource\select.bmp'.
.br
By pressing the selectbutton with the above shown image the program comes
into selectmode. This can also be acomplished by pressing the <Esc>
key on the keyboard.
.br
If you have drawn some objects like circles, squares etc you can
stop with the creation of new objects by pressing the selectbutton.
If you now click on an object the program will show arround the object
it's selection handles. The program has made a single selection.
:artwork align=left name='..\help\handlx.bmp'.
.br
When an object is selected it can be moved with the mouse.
:ol compact.
:li.Click with the mouse (use left mouse button) on the object you want
to move and hold down the mouse button.
:li.Move the mouse.
:li.The outline of the object will move with the mouse pointer until the
mouse button is released.
:eol.
The size of the selected object can be changed in the following way.
.br
:ol compact.
:li.Move the mouse pointer above on of the selection handles. The shape of the
mouse pointer will change if you move over the handle.
:li.If the shape of the mouse pointer shows two arrows press the mouse button
and start moving the mouse.
:li.When the mouse is released the object is drawn in it's final size.
:eol.
.br
:hp2.Multiple Selections:ehp2.
You can select more than on object at a time in  two different ways.
.br
:ol compact.
:li.Click with the mouse outside an object and hold down the mouse button.
:li.Start moving the mouse. The program draws a selectionbox. All objects
which fall in the selection box will show their selection handles.
:li.Release the mouse button.
:li.The program has made a multiple selection.
:eol.
.br
:hp2.Multiple Selection (Second method):ehp2.
:ol compact.
:li.Press the <Shift> key and hold it down.
:li.Click with the mouse on an object and on a second,third etc object.
:li.Release the <Shift> key.
:li.The program has made a multiple selection.
:eol.
.br
Moving multiple objects works the same as for single objects.
.*--------------------------------------------------------------*\
.*  Indexhelp Additional objects   
.*      res = 3500
.*--------------------------------------------------------------*/
:h1 res=3500 name=PANEL_ADDINGOBJECTS.Additional objects
:i1. Additional objects
Objects can be added by clicking on one of the buttons, but there
are more objects. These additional objects can be found in the 
menu "Objects".
.br
.br
:hp2.Regular polystar:ehp2.
.br
When choosing this menu option the program will popup a small dialog
window enabling you to define the number of points used in the star.
The following example is created with the maximum number of points.
:artwork align=left name='..\help\regstar.bmp'.
The form of the figure can be changed with the following button.
.br
:artwork align=left name='..\resource\splhit.bmp'.
See for details :link reftype=hd res=2910.Getting started:elink.
.br
.br
:hp2.Partial Arc:ehp2.
.br
This circle is drawn partially. The radius is define
by the movement of the mouse in the X-direction while
the sweep andgle is defined by the movement of the mouse in the
Y-direction.
:artwork align=left name='..\help\partarcx.bmp'.
.br
:hp2.Special lines:ehp2.
.br
Click with the mouse at the point you like to
start, move the mouse while holding down the button a little to
the right and make a circular movement with mouse. Release the mouse.
:artwork align=left name='..\help\speclinx.bmp'.
.br
:nt.Try to use the cursor keys instead of the mouse, when an object 
needs to be adjusted for just one unit:ent.
.*--------------------------------------------------------------*\
.*  Indexhelp Aligning objects   
.*      res = 3600
.*--------------------------------------------------------------*/
:h1 res=3600 name=PANEL_ALIGNEOBJECTS.Align objects
:i1. Align objects
DrawIt supports six different alignment options.
.br
The first selected object functions as the reference point. When
you want to allign objects, start the multiple 
:link reftype=hd res=3400.selection:elink. on the object which you want 
to use as the reference point.
.br
Examples of non aligned objects.
:artwork align=left name='..\help\align1.bmp'.
.br
Example of horizontal aligned object.
:artwork align=left name='..\help\alignh.bmp'.
.*--------------------------------------------------------------*\
.*  Indexhelp PolyLines          
.*      res = 3700
.*--------------------------------------------------------------*/
:h1 res=3700 name=PANEL_POLYLINES.Polylines
:i1. Polylines    
:artwork align=left name='..\resource\spline.bmp'.
.br
Deleting a line while drawing.
.br
While drawing a polyline the last line can be deleted by pressing the
:hp2.minus key:ehp2. on the numeric keypad.
.br
When the :hp2.Enter:ehp2. key is pressed during the creation of the spline 
than the spline will be closed.
.br
After you have switched to selectmode e.g you pressed the ESC 
key or the select button, you can move and resize the polyline.
If the polyline is selected you can easily change the color by
clicking on the appropriate color in the colorpalette at the
bottom of the application, or you can drop a color from the
OS/2 colorpalette on the polyline.
If you double click with the left mouse button on the polyline
you get a dialog which enables you to transport the figure to a
different 
:link reftype=hd res=3800.layer:elink.
via the spinbutton. Further more two radio buttons enables you to
set the spline in it's default or closed state. Try the different 
settings to see what happens.
When the polyline is drawn on the screen and the program is switched
into select mode the polyline can be sized move and its shape can be editted.
For the latter you have to select the polygon (it shows the four black
boxes) and press the button as shown here.
:artwork align=left name='..\resource\splhit.bmp'.
For example if you create a regular polystar with 12 points the polystar will
show on each point an open square when this button is pressed.
:artwork align=left name='..\help\poly1.bmp'.
Each individual vertex can be moved by clicking in one of the small boxes and
drag the mouse (keep left button pressed ). When the mouse button is released 
the the polygon is redrawn in its new shape. The selected vertex will appear
as a small solid box. When now the delete key is pressed , this vertex will be
deleted. Also when pressing the control key and clicking on one of the lines
of the polygon a point will be added.
.br

.*--------------------------------------------------------------*\
.*  Help on layer dialog
.*      res = PANEL_LAYERDIALOG
.*--------------------------------------------------------------*/
:h1 res=3800 name=PANEL_LAYERDIALOG.Layer dialog
:artwork align=left name='..\resource\layer.bmp'.
To be able to set explicitly one object on the other, we introduced layer
support. You can draw your objects on 10 different layers. Layer one
is the faremost layer. To change the current layer you click on
the button with the above shown picture. Layers which are not active
or the default can be 'froozen'. This means you cannot select the objects
which are on the other layer than the current.
:hp8.By default the objects on the other layers cannot be selected, so
an export of the selected area as metafile or bitmap will only contain
the active layer!! :ehp8. But if you unselect the checkbox in the layer dialog
which 'unfreezes' the other layers than all objects can be selected and
so an export of the selection will contain all objects within that
area.
:nt. Double click on a selected object to move the object to another
layer. :ent.
.*--------------------------------------------------------------*\
.*  Help on gradient fill
.*      res = PANEL_GRADIENT
.*--------------------------------------------------------------*/
:h1 res=3900 name=PANEL_GRADIENT.Gradient fill
:i1. Gradient fill    
:artwork align=left name='..\help\hgradien.bmp'.
The color sheme we used is derived from the color wheel of OS/2.
If you go within OS/2 to system setup and start up one of the color
palette's you can see the colorwheel when you double click on one
of the fixed colors. The outer part of the circle is used by drawit.
So if you define a range of 360 degrees and a starting point at zero,
you see that you start in red and end in purple. So you run around
the a virtual colorwheel within drawit. The precision of the color
gradient will increase when smaller ranges are used.
.br
1. Go in the options menu to gradient, which will bringup the color
gradient dialog.
.br
2. Define a range and starting point to define the required gradient.
.br
3. Define the starting point of the gradient, this can be : bottom, top,
left and right.
.br
4. Define the color saturation which lies between 10% and 100%.
.br
5. Click on O.K.
.br
Be sure that the filling menu is checked on Gradient fill.
.br
Now every object which is drawn will be filled with the gradient you choose.
.br
When the program is in select mode objects can get gradient fill by selecting
the object and setting the fillingpattern in the filling menu to gradient fill.
Even if the option is checked you must activate it to change the selected object
it's filling pattern!
.br
:hp2.Suppress gradientfill.:ehp2.
.br
This menu option causes the program to suppress the gradient fill 
during drawing on the screen. This can be handy since drawing performance
increase especially when gradient filled parts are on top of other gradient
filled parts which needs a lot of ajustments and thus causes many repaints. 
The gradient fill will always be printed this option only influences the 
appearance on the screen. 
.br
:artwork align=left name='..\help\txtgrad.bmp'.
.br
:nt. Not all graphics cards do support this option. Keep this in mind
if the drawing needs to be used on other machines :ent.
.*--------------------------------------------------------------*\
.*  Help on grouping
.*      res = PANEL_GROUP
.*--------------------------------------------------------------*/
:h1 res=4000 name=PANEL_GROUP.Group and UnGroup
:i1. Group and UnGroup
Group
.br
Grouping objects makes it possible to treat a collection of objects if
it is one. Creating a group needs the following steps:
.br
1. Set the program in selectmode by pressing the select button
.br
:artwork align=left name='..\resource\select.bmp'.
.br
2. Select more than one object by moving the mouse while holding down
the left mouse button. The program draws a selectionbox. 
.br
3. If all wanted objects like circles,sqaures and other groups are 
within the selectbox, select in the Edit menu the Group option.
.br
4. Click on the group in the drawing area and see what happens.....
.br
:nt.Goups within groups is unlimited supported!:ent.
.br
UnGroup.
.br
1. Select the group which must be ungrouped.
.br
2. Select in the Edit menu the UnGroup option.
.br
3. Try to select one of the individual objects and look....
.br
:nt.Since Goups within groups are supported it can be the case that there
are still groups left. The program UnGroups one level at a time.:ent.
.br
More detials:
.br
Since a group can consist of objects with different colors and filling patterns,
it is not possible to change the color of the individual items in a group. If
one item must be recolored or put on a different layer, you have to ungroup first
before actions can be done on one of the elements in the group. 
A group can also not be transported to a different layer.
.*--------------------------------------------------------------*\
.*  Help on clipboard
.*      res = PANEL_CLIPBOARD
.*--------------------------------------------------------------*/
:h1 res=4100 name=PANEL_CLIPBOARD.Clipboard
:i1. Clipboard
:artwork align=left name='..\help\clip.bmp'.
.br
The clipboard functions in the edit menu only supports bitmap and
metafile objects.
.br
Copy:
.br
1. Set the program in selectmode.
.br
2. Select the image which you want to copy to the clipboard.
.br
3. Select the copy function in the edit menu or press the key's Ctrl+Insert.
.br
Paste:
.br
1. Set the program in select mode.
.br
2. Select the Paste option in the edit menu or press the key's Shif+Insert.
.*--------------------------------------------------------------*\
.*  Help on Drag and Drop
.*      res = PANEL_DRAGDROP 
.*--------------------------------------------------------------*/
:h1 res=4200 name=PANEL_DRAGDROP.DragDrop 
:i1. DragDrop
As you have seen with normal objects like circles, squares, text etc,
you can change the color by dropping a color from the OS/2 color
pallete directly on it. Also a color change on multiple objects
can be done when the objects are in a multiselection box. Of course
you can do this also with the normal program pallete which lives at
the bottom of the DrawIt window.
.br
1. Draw a circle.
.br
2. Press the select button.
:artwork align=left name='..\resource\select.bmp'.
.br
3. Press the colorpallete button.
:artwork align=left name='..\resource\fill.bmp'.
.br
4. Drag a color to the circle object and release the mouse button.
.br
.br
Also the font can be changed of a text object or of all selected text
objects within a multiselect, just by dropping a font from the OS/2
font pallete on it.
.br
1. Type some text like Hello World.
.br
2. Press the select button.
:artwork align=left name='..\resource\select.bmp'.
3. Press the font pallete button.
:artwork align=left name='..\resource\font.bmp'.
4. Drag a font from the font pallete on the text and see what
happens.
.br
.br
:hp2.Picture dropping. :ehp2.
.br
You can also drop directly from the filemanager or from the famous
Explorer (WWW browser) or any other program which supports drag and
drop, pictures on DrawIt. If the picture format
is supported, DrawIt will show you directly the picture!
.*--------------------------------------------------------------*\
.*  Help on circular text
.*      res = PANEL_CIRCULARTEXT
.*--------------------------------------------------------------*/
:h1 res=4300 name=PANEL_CIRCULARTEXT.CircularText
:i1. CircularText
When you have selected a text object, the Circular option in the text
menu is enabled. With this option you can startup the dialog which
enables you to set the text to circular.
.br
You can set the circular attribute of the selected text on or off
with the checkbox.
Standard the text starts at 180 degrees ( at nine o clock ) and ends
at 180 degrees since the sweep angle is 360 degrees, which means a
full circle.
.br
The starting angle can be changed with the most left slider. By
changing the slider position the start angle of the text will
be changed. The result can be seen in the window at the right of
the dialog. The second slider enables you to define the sweep angle
of the text. 
.br
As an example we can make circular text which describes half of the
circle at the top.
.br
1.Set the first slider which defines the start angle on 180.
.br
2.Set the second slider which defines the sweep angle to 180 degrees.
.br
3.Switch the check box on. It shows a v mark when it is on.
.br
4.Press the OK button.
.br
:artwork align=left name='..\help\circular.bmp'.
.br
:hp2.Example 2:ehp2.
:artwork align=left name='..\help\circtxt.bmp'.
.*--------------------------------------------------------------*\
.* Help for printer setup
.*      res = PANEL_QUERYPRINT
.* -----------------------------------------------------------------
:h1 res=4500 name=PANEL_QUERYPRINT.Help for Printing options
:p.
Select from the listbox the printer you want to use for printing.
:p.
Use the :hp2.Job properties...:ehp2. pushbutton to start a dialog
from which you can control your printing configuration.
.*--------------------------------------------------------------*\
.* Help for export selection to PCX,GIF,TIF and TGA...
.*      res = PANEL_EXPORTSEL2BMP
.* -----------------------------------------------------------------
:h1 res=4600 name=PANEL_EXPORTSEL2BMP.Export selection
:hp2.Export selection / Export selected Image:ehp2.
.br
In this dialog you can select the format to use when the image
is written to disk. The question of course is why all those different
formats?? The reason is that you may want to use the exported image
in another application. When you use this dialog to export
a selection of figures it is possible to save it in all number of
colors. Except in the GIF format. This format does not support true
color. When you use this dialog to save the selected image the 
number of colors in the selected image determines the number of colors
you can select for export.
.br
Example, the 256 color version is supported by PC Paintbrush.
.br
When the whole drawing is exported as bitmap image, both spinbuttons
are filled in with the bitmap size. The initial size in the fields
are such that the aspect ratio of the drawing is preserved. 
:nt.True color images take a lot of diskspace.:ent.
.*------------------------------------------------------------------
.* Help for keyboard
.*      res = PANEL_KEYBOARD
.* -----------------------------------------------------------------
:h1 res=4700 name=PANEL_KEYBOARD.Keyboard
:i1. Keyboard
:artwork align=left name='..\help\keyb.bmp'.
.br
:hp2.<Ctrl> + B:ehp2.
.br
Bring up the file dialog for loading a bitmap.
.br
.br
:hp2.<Ctrl> + M:ehp2.
.br
Bring up the file dialog for loading an OS/2 Metafile.
.br
.br
:hp2.<Ctrl> + S:ehp2.
.br
Save current drawing.
.br
.br
:hp2.<Ctrl> + T:ehp2.
.br
When a text object is selected an editbox will come up to enable to
change the text of the text object.
.br
.br
:hp2.<Ctrl> + O:ehp2.
.br
Open a new file.
.br
.br
:hp2.<Shift>:ehp2.
.br
:ol compact.
:li.Used for making multiple selections in conjunction with the left mouse
button.
:li.To preserve the aspect ratio when drawing a circle or a square.
:eol.
.br
:hp2.<Ctrl>:ehp2.
.br
Use this key together with the left mouse button to make a copy of 
the current selected object (Not for images and metafiles).
.br
.br
:hp2.B:ehp2.
.br
When an image is selected this causes the incrementation of the 
brightness.
.br
.br
:hp2.I:ehp2.
.br
Inverts the selected image.
.br
.br
:hp2.<Delete>:ehp2.
.br
Delete the current selected object(s).
.br
.br
:hp2.+:ehp2.
.br
Plus key on the num keypad is used for color rotation in the selected
image. Increments color values.
The minus key on the num keypad is used for color rotation in the selected
image. Decrements color values.
.br
.br
:hp2.Cursor keys:ehp2.
.br
Move the selected object with one unit.
.*--------------------------------------------------------------*\
.* Help for object rotation
.*      res = PANEL_ROTATION
.* -----------------------------------------------------------------
:h1 res=4800 name=PANEL_ROTATION.Rotation
:i1. Rotation
:artwork align=left name='..\resource\rotate.bmp'.
.br
Select the object you want to rotate and press the button with the picture
as shown above this text. The program will show a small dialog window in
the upperright corner telling you how many degrees the object is rotated
and the interval to snap on while you are rotating the object.
The selected object will show its rotation center as a small circle and
a rotation handle at the right side of the object.
:artwork align=left name='..\help\rotex.bmp'.
.br
Click with the mouse on the rotation handle and start moving the mouse
while holding down the mouse button. The figure will rotate around the
center handle. Release the mouse button to fix the object.
.br
Move the rotation centre when the object needs to be rotated around 
another point. Click on the rotation centre and move the mouse
while holding down the button. Try to rotate the object again and see the
result.
.br
.br
:hp2.An example:ehp2.
:artwork align=left name='..\help\rotey.bmp'.
Draw a circle with a small circle on the right hand side of the big one.
Select the small circle and press the rotation button. The small circle
shows it's rotation handle and it rotation centre. Click on the rotation
centre and move the mouse to the center of the big circle. Release the mouse
to set the rotation centre. Set the snap on degrees on 45 degrees. 
Press the Ctrl key and rotate the small circle around the big circle. Each
time the small circle jumps to its new location (45 degrees further) release
the mouse button. Keep the Ctrl key pressed and rotate the new circle again.
After the eightth circle is drawn, release the mouse button. 
.br
:hp2.Example 2:ehp2.
.br
The following figure shows text which is rotated around a circle. The snap
on degrees was set on 45.
:artwork align=left name='..\help\rotez.bmp'.
.br
:hp2.Example 3:ehp2.
.br
:artwork align=left name='..\help\bloem.bmp'.
.*--------------------------------------------------------------*\
.* Help for object rotation
.*      res = PANEL_FORMSIZE
.* -----------------------------------------------------------------
:h1 res=4900 name=PANEL_FORMSIZE.Formsize
:i1.Formsize
:artwork align=left name='..\help\form.bmp'.
This dialog allows you to change the formsize where you draw on. It does
not represent the forms which are supported by your printer. If
you choose a certain formsize be sure that your printer supports this form.
When there is a mismatch between the formsize on the screen and the printer
the drawing will look different when printed. This dialog allows
you to setup the program in such a way that the printout will get
an almost identical look when it is printed on your printer.
When the drawing is saved to disk the forminfo is saved with the drawing.
Loading the drawing will adjust the formsize on the screen when necessary
If the file is inserted in an existing drawing, the first loaded drawing 
determines the size of the form. The program will give a warning when 
there is a mismatch.
:table cols='15 15 15'.
:row.
:c.Form name
:c.Inch
:c.Centimeter
:row.
:c.A4
:c.8&dot.27 &asterisk. 11&dot.79
:c.21&comma.00 &asterisk. 29&comma.96
:row.
:c.Letter
:c.8&dot.5 &asterisk. 11
:c.21&comma.59 &asterisk. 27&comma.94
:row.
:c.Legal
:c.8&dot.5 &asterisk. 14
:c.21&comma.59 &asterisk. 35&comma.56
:row.
:c.Executive
:c.7&dot.25 &asterisk. 10&dot.5
:c.18&comma.42 &asterisk. 26&comma.67
:row.
:c.C5 envelope
:c.6&dot.38 &asterisk. 9&dot.01
:c.16&comma.20 &asterisk. 22&comma.89
:row.
:c.DL envelope
:c.4&dot.33 &asterisk. 8&dot.66
:c.11&comma.00 &asterisk. 22&comma.00
:row.
:c.COM&dash.10 evelope
:c.4&dot.12 &asterisk. 9&dot.50
:c.10&comma.47 &asterisk. 24&comma.13
:row.
:c.Monarch envelope
:c.3&dot.87 &asterisk. 7&dot.50
:c.9&comma.84 &asterisk. 19&comma.05
:etable.
.*--------------------------------------------------------------*\
.* Help for object rotation
.*      res = PANEL_FORMSIZE
.* -----------------------------------------------------------------
:h1 res=5000 name=PANEL_PICFILE.Lotus 123 Picfile.
DrawIt supports the loading of Lotus 123 chart files. This file format
is used until Lotus 123 3.0. But the modern versions of Lotus 123 are
still able to save the chart in the old format.
.br
The fonts can be changed for both the header text and the regular text
in the chart. The line type can be changed for all type of charts. The filling
pattern does not change the filling of a pie chart.
.br
:nt.Lotus is a trademark of Lotus Development Corp. :ent.
.*--------------------------------------------------------------*\
.* Help for balloon text
.*      res = 
.* -----------------------------------------------------------------
:h1 res=5100 name=PANEL_BALLOON.Balloon text example.
:artwork align=left name='..\resource\trian.bmp'.
Use the above shown button to start the creation of a poly point. Set the
number of points in the small window at its maximum value.
Now when you start drawing you see that the shape is circular.
Press the ESC key or the select button.
:artwork align=left name='..\resource\splhit.bmp'.
Select the circle and press the above shown button. Now all the points
are visible. Drag one point away and have a look!
.br
:artwork align=left name='..\help\balloon.bmp'.
:euserdoc.
