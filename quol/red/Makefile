# red top-level Makefile

include red.gmake

DIRS := lib tools test
.PHONY: $(DIRS)

all: $(DIRS:%=all-%)

sure: lib
	$(MAKE) -C test $@

clean: $(DIRS:%=clean-%)

depend: $(DIRS;%=depend-%)

tools: lib

test: lib

$(DIRS):
	make -C $@

all-%:
	make -C $* all

clean-%:
	make -C $* clean
