# red tools Makefile

include ../red.gmake

CPPS := $(wildcard *.cpp)
EXES := $(CPPS:.cpp=)

all: $(EXES)

clean:
	$(RM) $(EXES) *.o *.d

ifneq ($(wildcard *.d), )
  include *.d
endif
