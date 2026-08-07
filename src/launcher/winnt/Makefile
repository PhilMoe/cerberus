CC      := gcc
CXX     := g++
LD      := g++

RES     := windres

CFLAGS   :=
CXXFLAGS :=
CPPFLAGS := $(CC_OPTS) -fpic -s
LDFLAGS  := $(LD_OPTS) -fpic

ifeq ($(STATIC_LINK),1)
LDLIBS := -static-libgcc -static-libstdc++ \
          -Wl,-Bstatic -lstdc++ -lpthread \
          -Wl,-Bdynamic $(LD_LIB_OPTS) \
          -lwinmm -lws2_32 -ladvapi32
else
LDLIBS := $(LD_LIB_OPTS) -lwinmm -lws2_32 -ladvapi32
endif

RES_ARCH := pe-x86-64
ifeq ($(MSIZE),32)
RES_ARCH := pe-i386
endif

PARENT_DIR := ..

TMP := $(BUILD_DIR)/$(dir $(OUT))

OBJS := \
    $(TMP)launcher.o \
    $(TMP)resource.o

all: $(OUT_PATH)/$(OUT)

$(OUT_PATH)/$(OUT): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

#
# Compile launcher.cpp
#
$(TMP)launcher.o: launcher.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

#
# Compile resource.rc
#
$(TMP)resource.o: $(PARENT_DIR)/resource.rc
	$(RES) --target=$(RES_ARCH) \
	       --input-format=rc \
	       --output-format=coff \
	       -I$(PARENT_DIR) \
	       -i $< \
	       -o $@