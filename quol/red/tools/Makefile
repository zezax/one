# red tools Makefile

ROOT := ..
SUB := tools

include ../red.gmake

.PRECIOUS: $(BUILDSUB)/%.o

CPPS := $(wildcard *.cpp)
EXES := $(CPPS:%.cpp=$(BUILDSUB)/%)

all: $(EXES)

clean:
	$(RM) -r $(BUILDSUB)

ifneq ($(wildcard $(BUILDSUB)/*.d), )
  include $(BUILDSUB)/*.d
endif
