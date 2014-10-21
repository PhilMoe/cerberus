
CC=gcc
CFLAGS=

CXX=g++
CXXFLAGS=

CPPFLAGS=$(CCOPTS) \
-Wno-free-nonheap-object \
-Wno-unused-result \
-I../glfw3/include \
-I../openal/include \
-I../stb \
-D_GLFW_HAS_GLXGETPROCADDRESS \
-pthread

LD=g++
LDFLAGS=
LDLIBS=-lGL -lopenal -lX11 -lXxf86vm -lXi -lXrandr -lXinerama -lXcursor -lpthread

OBJS=\
../glfw3/src/context.o \
../glfw3/src/init.o \
../glfw3/src/input.o \
../glfw3/src/joystick.o \
../glfw3/src/monitor.o \
../glfw3/src/time.o \
../glfw3/src/window.o \
../glfw3/src/glx_context.o \
../glfw3/src/x11_init.o \
../glfw3/src/x11_monitor.o \
../glfw3/src/x11_window.o \
../glfw3/src/posix_time.o \
../glfw3/src/posix_tls.o \
../glfw3/src/linux_joystick.o \
../glfw3/src/xkb_unicode.o \
../stb/stb_vorbis.o \
../stb/stb_image.o \
../main.o

all : $(OUT)

$(OUT) : $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
