CC=gcc
CFLAGS=

CXX=g++
CXXFLAGS=

CPPFLAGS=$(CCOPTS) -fpic

LD=g++
LDFLAGS=$(LDOPTS) -fpic
LDLIBS=$(LIBOPTS) -lwinmm -lws2_32

VPATH= $(VPATHOPTS) \
%.cpp ..

OBJS0= $(SRCOPTS) \
main.o

TMP=$(BUILD_DIR)/$(dir $(OUT))

OBJS=$(patsubst %.o,$(TMP)%.o,$(OBJS0))
all : $(OUT_PATH)/$(OUT)

$(OUT_PATH)/$(OUT) : $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(TMP)%.o : %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<