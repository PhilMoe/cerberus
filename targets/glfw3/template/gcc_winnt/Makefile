CC=gcc
CFLAGS=-D_GLFW_WIN32

CXX=g++
CXXFLAGS=

CPPFLAGS=$(CCOPTS) \
-Wno-free-nonheap-object \
-I../curl/include \
-I../glfw3/include \
-I../glfw3/src \
-I../openal/include \
-I../stb \
-I../tinyfiledialogs \
-I../zlib-1.2.8 \
-I../lpng1610

LD=g++
LDFLAGS=$(LDOPTS) -Wl,--subsystem,windows
LDLIBS=-lcomdlg32 -lgdi32 -lopengl32 $(LIBOPTS) -lws2_32 -lole32

RES=windres
ifeq ($(ARCH),64)
    RES_ARCH=pe-x86-64
else
    RES_ARCH=pe-i386
endif

VPATH= $(VPATHOPTS) \
%.c ../glfw3/src \
%.c ../stb \
%.c ../tinyfiledialogs \
%.cpp .. \
%.rc ..

OBJS0= $(SRCOPTS) \
context.o \
egl_context.o \
init.o \
input.o \
monitor.o \
osmesa_context.o \
vulkan.o \
wgl_context.o \
win32_init.o \
win32_joystick.o \
win32_monitor.o \
win32_time.o \
win32_thread.o \
win32_window.o \
window.o \
stb_vorbis.o \
stb_image.o \
resource.o \
tinyfiledialogs.o \
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
	
$(TMP)%.o : %.rc
	$(RES) -O COFF --target $(RES_ARCH) -i $< -o $@
	