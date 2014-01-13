
CC=gcc
CFLAGS=

CXX=g++
CXXFLAGS=

CPPFLAGS=$(CCOPTS) -I../glfw/include -I../glfw/lib -I../glfw/lib/win32 -I../openal/include -I../stb

LD=g++
LDFLAGS=-Wl,--subsystem,windows -L../openal/libs/Win32
LDLIBS=-lgdi32 -lopengl32 -lOpenAL32 -lws2_32

OBJS=../glfw/lib/enable.o ../glfw/lib/fullscreen.o ../glfw/lib/glext.o ../glfw/lib/image.o ../glfw/lib/init.o ../glfw/lib/input.o \
../glfw/lib/joystick.o ../glfw/lib/stream.o ../glfw/lib/tga.o ../glfw/lib/thread.o ../glfw/lib/time.o ../glfw/lib/window.o \
../glfw/lib/win32/win32_dllmain.o ../glfw/lib/win32/win32_enable.o ../glfw/lib/win32/win32_fullscreen.o ../glfw/lib/win32/win32_glext.o \
../glfw/lib/win32/win32_init.o ../glfw/lib/win32/win32_joystick.o ../glfw/lib/win32/win32_thread.o ../glfw/lib/win32/win32_time.o ../glfw/lib/win32/win32_window.o \
../stb/stb_image.o ../stb/stb_vorbis.o ../main.o

all : $(OUT)

$(OUT) : $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
	