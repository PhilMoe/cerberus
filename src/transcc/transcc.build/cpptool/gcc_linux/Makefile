CC=$(C_COMPILER)
CFLAGS=

CXX=$(CXX_COMPILER)
CXXFLAGS=

CPPFLAGS=$(CC_OPTS)

LD=g++
LDFLAGS=$(LD_OPTS)
LDLIBS=$(LD_LIB_OPTS)

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
