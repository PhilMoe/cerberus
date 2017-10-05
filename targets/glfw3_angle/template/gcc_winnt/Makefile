
CC=gcc
CFLAGS=

CXX=g++
CXXFLAGS=

CPPFLAGS=$(CCOPTS) \
-Wno-free-nonheap-object \
-I../angle/include \
-I../glfw3/include \
-I../glfw3/src \
-I../openal/include \
-I../stb \
-I../zlib-1.2.8 \
-I../lpng1610

LD=g++
LDFLAGS=$(LDOPTS) -Wl,--subsystem,windows -L../angle/lib
LDLIBS=-llibEGL -llibGLESv2 -lcomdlg32 -lgdi32 $(LIBOPTS) -lws2_32

vpath %.c ../glfw3/src
vpath %.c ../stb
vpath %.cpp ..

OBJS0=\
context.o \
init.o \
input.o \
monitor.o \
egl_context.o \
win32_init.o \
win32_monitor.o \
win32_time.o \
win32_tls.o \
win32_window.o \
window.o \
winmm_joystick.o \
stb_vorbis.o \
stb_image.o \
main.o

TMP=build/$(dir $(OUT))

OBJS=$(patsubst %.o,$(TMP)%.o,$(OBJS0))

all : $(OUT)
	copy ..\angle\bin\libEGL.dll $(subst /,\,$(dir $(OUT)))
	copy ..\angle\bin\libGLESv2.dll $(subst /,\,$(dir $(OUT)))
	copy ..\angle\bin\d3dcompiler_47.dll $(subst /,\,$(dir $(OUT)))

$(OUT) : $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(TMP)%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(TMP)%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
	
