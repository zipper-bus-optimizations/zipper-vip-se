
LOCAL_CFLAGS= -I../common
LOCAL_LIBS=

PROG=test
OBJS=test.o ../common/utils.o

include ../Makefile

test: $(TARGET_EXE)
	-$(TARGET_SIM) ./$(TARGET_EXE)

