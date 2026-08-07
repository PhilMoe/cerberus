# GLFW GCC MINGW MAKE FILE
CC=gcc
CFLAGS=-D_GLFW_WIN32

CXX=g++
CXXFLAGS=

# Compiler flags.
CPPFLAGS=$(CC_OPTS) \
-Wno-free-nonheap-object \
-I../curl/include \
-I../glfw3/include \
-I../glfw3/src \
-I../openal/include \
-I../stb \
-I../tinyfiledialogs \
-I../zlib-1.2.8 \
-I../lpng1610

# Linker flags
LD=g++
LDFLAGS=$(LD_OPTS) -Wl,--subsystem,windows
ifeq ($(STATIC_LINK),1)
LDLIBS=-static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic -lcomdlg32 -lgdi32 -lopengl32 $(LD_LIB_OPTS) -lws2_32 -lole32
else
LDLIBS=-lcomdlg32 -lgdi32 -lopengl32 $(LD_LIB_OPTS) -lws2_32 -lole32
endif

# Set the resource compiler flags
RES=windres
RES_ARCH=pe-x86-64
ifeq ($(MSIZE),32)
   RES_ARCH=pe-i386 
endif

# Add virtual paths to search
VPATH= $(VPATH_OPTS) \
%.c ../glfw3/src \
%.c ../stb \
%.c ../tinyfiledialogs \
%.cpp .. \
%.rc ..

# Add source files.
OBJS0= $(SRC_OPTS) \
context.o \
egl_context.o \
init.o \
input.o \
monitor.o \
null_init.o \
null_joystick.o \
null_monitor.o \
null_window.o \
osmesa_context.o \
platform.o \
vulkan.o \
wgl_context.o \
win32_init.o \
win32_joystick.o \
win32_module.o \
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

# Setup build directory
TMP=$(BUILD_DIR)/$(dir $(OUT))

# File subsitution
OBJS=$(patsubst %.o,$(TMP)%.o,$(OBJS0))

# Final build output
all : $(OUT_PATH)/$(OUT)

$(OUT_PATH)/$(OUT) : $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(TMP)%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(TMP)%.o : %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
	
$(TMP)%.o : %.rc
	$(RES) -O COFF --target $(RES_ARCH) -i $< -o $@
	