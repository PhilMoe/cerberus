
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
-D_GLFW_HAS_GLXGETPROCADDRESS \
-pthread

LD=g++
LDFLAGS=
LDLIBS=-lGL -lopenal -lX11 -lXxf86vm -lXi -lXrandr -lXinerama -lXcursor -lpthread -ldl

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
posix_tls.o \
linux_joystick.o \
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
