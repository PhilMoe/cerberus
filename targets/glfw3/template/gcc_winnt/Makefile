
CC=gcc
CFLAGS=

CXX=g++
CXXFLAGS=

CPPFLAGS=$(CCOPTS) \
-Wno-free-nonheap-object \
-I../glfw3/include \
-I../glfw3/src \
-I../openal/include \
-I../stb \
-I../zlib-1.2.8 \
-I../lpng1610

LD=g++
LDFLAGS=-Wl,--subsystem,windows -L../openal/libs/Win32 -L../openal/libs/Win64
LDLIBS=-lgdi32 -lopengl32 -lOpenAL32 -lws2_32

OBJS=\
../glfw3/src/context.o \
../glfw3/src/init.o \
../glfw3/src/input.o \
../glfw3/src/joystick.o \
../glfw3/src/monitor.o \
../glfw3/src/time.o \
../glfw3/src/wgl_context.o \
../glfw3/src/win32_init.o \
../glfw3/src/win32_monitor.o \
../glfw3/src/win32_time.o \
../glfw3/src/win32_tls.o \
../glfw3/src/win32_window.o \
../glfw3/src/window.o \
../glfw3/src/winmm_joystick.o \
../stb/stb_vorbis.o \
../stb/stb_image.o \
../main.o

all : $(OUT)

$(OUT) : $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
