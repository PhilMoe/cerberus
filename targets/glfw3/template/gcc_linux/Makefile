
CC=gcc
CFLAGS=

CXX=g++
CXXFLAGS=

CPPFLAGS=$(CCOPTS) \
-Wno-int-to-pointer-cast \
-Wno-free-nonheap-object \
-Wno-unused-result \
-I../glfw3/include \
-I../openal/include \
-I../stb \
-I../glfw3/deps \
-I../glfw3/deps/vulkan \
-D_GLFW_HAS_GLXGETPROCADDRESS \
-pthread

LD=g++
LDFLAGS=$(LDOPTS)
LDLIBS=-lGL -lX11 -lXxf86vm -lXi -lXrandr -lXinerama -lXcursor $(LIBOPTS) -lpthread -ldl 

vpath %.c ../glfw3/src
vpath %.c ../stb
vpath %.cpp ..

OBJS0=\
context.o \
init.o \
input.o \
monitor.o \
window.o \
glx_context.o \
x11_init.o \
x11_monitor.o \
x11_window.o \
posix_time.o \
posix_thread.o \
linux_joystick.o \
vulkan.o \
egl_context.o \
osmesa_context.o \
xkb_unicode.o \
stb_vorbis.o \
stb_image.o \
main.o

TMP=build/$(dir $(OUT))

OBJS=$(patsubst %.o,$(TMP)%.o,$(OBJS0))

all : $(OUT)

$(OUT) : $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(TMP)%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(TMP)%.o : %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
