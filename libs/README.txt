Dynamic link libraries and MinGW archive files should be placed in their architeture folders.
The use of .LIB files should be avoided as there is no guarantee that they will work on all compilers, even Visual Studio has
been know to have issues with these. It's better just to distribute a dynamic-link library or compile a static library with the compiler you are using.

NOTE:
Two types of licence file can be copied.
If you need to distribute a licence then use the libraries name appended with _LICENCE.
If you need to distribute a copying licence (i.e. LGPL) then append _COPYING.