CC=gcc
CFLAGS=

CXX=g++
CXXFLAGS=

CPPFLAGS=$(CC_OPTS) -fpic

LD=g++
LDFLAGS=$(LD_OPTS) -fpic

ifeq ($(STATIC_LINK),1)
LDLIBS=-static-libgcc -static-libstdc++ -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic $(LD_LIB_OPTS) -lwinmm -lws2_32
else
LDLIBS=$(LD_LIB_OPTS) -lwinmm -lws2_32
endif

VPATH= $(VPATH_OPTS) \
%.cpp ..

OBJS0= $(SRC_OPTS) \
main.o

TMP=$(BUILD_DIR)/$(dir $(OUT))

OBJS=$(patsubst %.o,$(TMP)%.o,$(OBJS0))
all : $(OUT_PATH)/$(OUT)

$(OUT_PATH)/$(OUT) : $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(TMP)%.o : %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<