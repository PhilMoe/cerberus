CC=gcc
CFLAGS=-D_GLFW_X11

CXX=g++
CXXFLAGS=

CPPFLAGS=$(CCOPTS) \
-Wno-int-to-pointer-cast \
-Wno-free-nonheap-object \
-Wno-unused-result \
-I../curl/include \
-I../glfw3/include \
-I../openal/include \
-I../stb \
-I../tinyfiledialogs \
-I../glfw3/deps \
-I../glfw3/deps/vulkan \
-D_GLFW_HAS_GLXGETPROCADDRESS \
-pthread

LD=g++
LDFLAGS=$(LDOPTS) -no-pie
LDLIBS=-lGL -lX11 -lXxf86vm -lXi -lXrandr -lXinerama -lXcursor $(LIBOPTS) -lpthread -ldl 

VPATH= $(VPATHOPTS) \
%.c ../glfw3/src \
%.c ../stb \
%.c ../tinyfiledialogs \
%.cpp ..

OBJS0= $(SRCOPTS) \
context.o \
egl_context.o \
glx_context.o \
init.o \
input.o \
linux_joystick.o \
monitor.o \
null_init.o \
null_joystick.o \
null_monitor.o \
null_window.o \
osmesa_context.o \
platform.o \
posix_module.o \
posix_poll.o \
posix_thread.o \
posix_time.o \
stb_vorbis.o \
stb_image.o \
tinyfiledialogs.o \
vulkan.o \
wgl_context.o \
window.o \
wl_init.o \
wl_monitor.o \
wl_window.o \
x11_init.o \
x11_monitor.o \
x11_window.o \
xkb_unicode.o \
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
