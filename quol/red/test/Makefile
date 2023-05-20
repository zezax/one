# red/test Makefile

ROOT := ..
SUB := test

include ../red.gmake

.PRECIOUS: $(BUILDSUB)/%.o

CPPS := $(wildcard *.cpp)
EXES := $(CPPS:%.cpp=$(BUILDSUB)/%)
TESTS := $(CPPS:%.cpp=test-%)

LDLIBS += -lgtest_main -lgtest -lpthread

all: $(EXES)

sure: $(TESTS)

clean:
	$(RM) -r $(BUILDSUB)

test-%: $(BUILDSUB)/%
	$(BUILDSUB)/$*

ifneq ($(wildcard $(BUILDSUB)/*.d), )
  include $(BUILDSUB)/*.d
endif
