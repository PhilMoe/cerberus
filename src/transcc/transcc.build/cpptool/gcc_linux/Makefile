CC=$(C_COMPILER)
CFLAGS=

CXX=$(CXX_COMPILER)
CXXFLAGS=

CPPFLAGS=$(CCOPTS)

LD=g++
LDFLAGS=$(LDOPTS)
LDLIBS=$(LIBOPTS)

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
