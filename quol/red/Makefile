# red top-level Makefile

include red.gmake

DIRS := lib tools test
.PHONY: $(DIRS)

all: $(DIRS:%=all-%)

sure: lib
	$(MAKE) -C test $@

clean: $(DIRS:%=clean-%)
	$(RM) perf.data

depend: $(DIRS;%=depend-%)

tools: lib

test: lib

$(DIRS):
	$(MAKE) -C $@

all-%:
	$(MAKE) -C $* all

clean-%:
	$(MAKE) -C $* clean

all-tools: all-lib

all-test: all-lib
