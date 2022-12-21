# red library Makefile

include ../red.gmake

LIB := libred.a
CPPS := $(wildcard *.cpp)

all: $(LIB)

clean:
	$(RM) $(LIB) *.o *.d

$(LIB): $(CPPS:.cpp=.o)
	$(AR) $(ARFLAGS) $@ $?

ifneq ($(wildcard *.d), )
  include *.d
endif
