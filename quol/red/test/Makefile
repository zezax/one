# red/test Makefile

include ../red.gmake

CPPS := $(wildcard *.cpp)
EXES := $(CPPS:.cpp=)
TESTS := $(EXES:%=test-%)

LDLIBS += -lgtest_main -lgtest

all: $(EXES)

sure: $(TESTS)

clean:
	$(RM) $(EXES) *.o *.d

test-%: %
	./$*

ifneq ($(wildcard *.d), )
  include *.d
endif
