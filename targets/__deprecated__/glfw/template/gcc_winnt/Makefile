
CC=gcc
CFLAGS=

CXX=g++
CXXFLAGS=

CPPFLAGS=$(CCOPTS) \
-Wno-free-nonheap-object \
-I../glfw/include \
-I../glfw/lib \
-I../glfw/lib/win32 \
-I../openal/include \
-I../stb

LD=g++
LDFLAGS=$(LDOPTS) -Wl,--subsystem,windows -L../openal/libs/Win32 -L../openal/libs/Win64
LDLIBS=-lgdi32 -lopengl32 -lOpenAL32 -lws2_32

vpath %.c ../glfw/lib
vpath %.c ../glfw/lib/win32
vpath %.c ../stb
vpath %.cpp ..

OBJS0=\
enable.o \
fullscreen.o \
glext.o \
image.o \
init.o \
input.o \
joystick.o \
stream.o \
tga.o \
thread.o \
time.o \
window.o \
win32_dllmain.o \
win32_enable.o \
win32_fullscreen.o \
win32_glext.o \
win32_init.o \
win32_joystick.o \
win32_thread.o \
win32_time.o \
win32_window.o \
stb_image.o \
stb_vorbis.o \
main.o

TMP=build/$(dir $(OUT))

OBJS=$(patsubst %.o,$(TMP)%.o,$(OBJS0))

all : $(OUT)

$(OUT) : $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(TMP)%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(TMP)%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

	