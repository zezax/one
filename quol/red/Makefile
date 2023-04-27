# red top-level Makefile

ROOT := .
SUB := .

include red.gmake

DIRS := lib tools test
.PHONY: $(DIRS)

all: $(DIRS:%=all-%)

sure: lib
	$(MAKE) -C test $@

clean:
	$(RM) -r $(BUILD)

distclean: clean
	find $(ROOT) -type f | egrep '(/core|/a.out|/perf[.]data([.]old)?|[.][ado]|~)$$' | xargs -r $(RM)

$(DIRS):
	$(MAKE) -C $@

all-%:
	$(MAKE) -C $* all

all-tools: all-lib

all-test: all-lib

cov:
	$(MAKE) -C lib $@

tidy:
	$(MAKE) -C lib $@
