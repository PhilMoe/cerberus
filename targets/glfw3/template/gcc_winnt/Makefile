
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
LDFLAGS=$(LDOPTS) -Wl,--subsystem,windows -L../openal/libs/Win32 -L../openal/libs/Win64
LDLIBS=-lcomdlg32 -lgdi32 -lopengl32 -lOpenAL32 -lws2_32

vpath %.c ../glfw3/src
vpath %.c ../stb
vpath %.cpp ..

OBJS0=\
context.o \
init.o \
input.o \
monitor.o \
wgl_context.o \
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

$(OUT) : $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(TMP)%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(TMP)%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
