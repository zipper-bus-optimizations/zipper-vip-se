include ../config.mk

MODE=enc
#
# END of user-modifiable variables
#

ifeq ($(MODE), na)
TARGET_CFLAGS = $(NA_CFLAGS)
TARGET_LIBS = $(NA_LIBS)
TARGET_SIM = $(NA_SIM)
TARGET_DIFF = $(NA_DIFF)
TARGET_EXE = $(PROG).na
else ifeq ($(MODE), do)
TARGET_CFLAGS = $(DO_CFLAGS)
TARGET_LIBS = $(DO_LIBS)
TARGET_SIM = $(DO_SIM)
TARGET_DIFF = $(DO_DIFF)
TARGET_EXE = $(PROG).do
else ifeq ($(MODE), enc)
TARGET_CFLAGS = $(ENC_CFLAGS)
TARGET_LIBS = $(ENC_LIBS)
TARGET_SIM = $(ENC_SIM)
TARGET_DIFF = $(ENC_DIFF)
TARGET_EXE = $(PROG).enc
else
# default is native
TARGET_CFLAGS = $(NA_CFLAGS)
TARGET_LIBS = $(NA_LIBS)
TARGET_SIM = $(NA_SIM)
TARGET_DIFF = $(NA_DIFF)
TARGET_EXE = $(PROG).na
endif

CFLAGS = -std=c++11 -Wall $(OPT_CFLAGS) -Wno-strict-aliasing $(TARGET_CFLAGS) $(LOCAL_CFLAGS)
LIBS = $(LOCAL_LIBS) $(TARGET_LIBS)

build: $(TARGET_EXE)

%.o: %.cpp
ifeq ($(MODE), na)
	$(CXX) $(CFLAGS) -DVIP_NA_MODE -o $(notdir $@) -c $<
else ifeq ($(MODE), do)
	$(CXX) $(CFLAGS) -DVIP_DO_MODE -o $(notdir $@) -c $<
else ifeq ($(MODE), enc)
	$(CXX) $(CFLAGS) -DVIP_ENC_MODE -o $(notdir $@) -c $<
else
	$(error MODE is not defined (add: MODE={na|do|enc}).)
endif

%.o: %.c
ifeq ($(MODE), na)
	$(CC) $(CFLAGS) -DVIP_NA_MODE -o $(notdir $@) -c $<
else ifeq ($(MODE), do)
	$(CC) $(CFLAGS) -DVIP_DO_MODE -o $(notdir $@) -c $<
else ifeq ($(MODE), enc)
	$(CC) $(CFLAGS) -DVIP_ENC_MODE -o $(notdir $@) -c $<
else
	$(error MODE is not defined (add: MODE={na|do|enc}).)
endif

$(TARGET_EXE): $(TARGET_EXE).fpga $(TARGET_EXE).ase $(TARGET_EXE)_NOREUSE.fpga $(TARGET_EXE)_NOREUSE.ase $(TARGET_EXE)_BASELINE.fpga $(TARGET_EXE)_BASELINE.ase

$(TARGET_EXE).fpga: $(OBJS)
	$(LD) $(CFLAGS) -o $@ $(notdir $^) $(LDFLAGS) $(CLINK_LIBFPGA)

$(TARGET_EXE).ase: $(OBJS)
	$(LD) $(CFLAGS) -o $@ $(notdir $^) $(LDFLAGS) $(CLINK_LIBASE)

$(TARGET_EXE)_NOREUSE.fpga: $(OBJS)
	$(LD) $(CFLAGS) -o $@ $(notdir $^) $(LDFLAGS) $(CLINK_LIBFPGA_NOREUSE)

$(TARGET_EXE)_NOREUSE.ase: $(OBJS)
	$(LD) $(CFLAGS) -o $@ $(notdir $^) $(LDFLAGS) $(CLINK_LIBASE_NOREUSE)

$(TARGET_EXE)_BASELINE.fpga: $(OBJS)
	$(LD) $(CFLAGS) -o $@ $(notdir $^) $(LDFLAGS) $(CLINK_LIBFPGA_BASELINE)

$(TARGET_EXE)_BASELINE.ase: $(OBJS)
	$(LD) $(CFLAGS) -o $@ $(notdir $^) $(LDFLAGS) $(CLINK_LIBASE_BASELINE)

clean:
	rm -f $(PROG).na $(PROG).do $(PROG).enc.* *.o core mem.out FOO $(LOCAL_CLEAN)

