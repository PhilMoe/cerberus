
CC=gcc
CFLAGS=

CXX=g++
CXXFLAGS=

CPPFLAGS=$(CCOPTS) \
 -I../glfw/include \
 -I../glfw/lib \
 -I../glfw/lib/x11 \
 -I../openal/include -I../stb \
 -D_GLFW_HAS_GLXGETPROCADDRESS \
 -D_GLFW_USE_LINUX_JOYSTICKS \
 -pthread

LD=g++
LDFLAGS=
LDLIBS=-lGL -lopenal -lX11 -lpthread

OBJS=../glfw/lib/enable.o ../glfw/lib/fullscreen.o ../glfw/lib/glext.o ../glfw/lib/image.o ../glfw/lib/init.o ../glfw/lib/input.o \
../glfw/lib/joystick.o ../glfw/lib/stream.o ../glfw/lib/tga.o ../glfw/lib/thread.o ../glfw/lib/time.o ../glfw/lib/window.o \
../glfw/lib/x11/x11_enable.o ../glfw/lib/x11/x11_fullscreen.o ../glfw/lib/x11/x11_glext.o \
../glfw/lib/x11/x11_init.o ../glfw/lib/x11/x11_joystick.o ../glfw/lib/x11/x11_thread.o ../glfw/lib/x11/x11_time.o ../glfw/lib/x11/x11_window.o \
../glfw/lib/x11/x11_keysym2unicode.o \
../stb/stb_image.o ../stb/stb_vorbis.o ../main.o

all : $(OUT)

$(OUT) : $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
