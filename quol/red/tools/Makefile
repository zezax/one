# red tools Makefile

ROOT := ..
SUB := tools

include ../red.gmake

.PRECIOUS: $(BUILDSUB)/%.o

NAMES := scan parse match serialize words bench big_red skim_red misc_red thr_red

ifdef HAS_RE2
  NAMES += big_re2 skim_re2 misc_re2 thr_re2
endif

EXES := $(NAMES:%=$(BUILDSUB)/%)

all: $(EXES)

clean:
	$(RM) -r $(BUILDSUB)

$(BUILDSUB)/big_re2: $(BUILDSUB)/big_re2.o $(DEPLIBS)
	$(CXX) $(LDFLAGS) $< -L$(BUILD)/lib -lred -lre2 $(LDLIBS) -o $@

$(BUILDSUB)/skim_re2: $(BUILDSUB)/skim_re2.o $(DEPLIBS)
	$(CXX) $(LDFLAGS) $< -L$(BUILD)/lib -lred -lre2 $(LDLIBS) -o $@

$(BUILDSUB)/misc_re2: $(BUILDSUB)/misc_re2.o $(DEPLIBS)
	$(CXX) $(LDFLAGS) $< -L$(BUILD)/lib -lred -lre2 $(LDLIBS) -o $@

$(BUILDSUB)/thr_re2: $(BUILDSUB)/thr_re2.o $(DEPLIBS)
	$(CXX) $(LDFLAGS) $< -L$(BUILD)/lib -lred -lre2 $(LDLIBS) -o $@

ifneq ($(wildcard $(BUILDSUB)/*.d), )
  include $(BUILDSUB)/*.d
endif
