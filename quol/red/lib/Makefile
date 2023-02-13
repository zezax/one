# red library Makefile

ROOT := ..
SUB := lib

include ../red.gmake

.PRECIOUS: $(BUILDSUB)/%.o

LIB := $(BUILDSUB)/libred.a
SO := $(BUILDSUB)/libred.so
CPPS := $(wildcard *.cpp)
OBJS := $(CPPS:%.cpp=$(BUILDSUB)/%.o)

ifeq ($(MODE), pic)

all: $(SO)

else

all: $(LIB)

endif

clean:
	$(RM) -r $(BUILDSUB)

cov:
	gcov -o $(BUILDSUB) -r -m $(CPPS) | sed -nEe '/^File/ {s/^File .(.*)./\1/; s:../include/::; h}; /^Lines/ {H; x; s/(.*)\nLines executed:(.*) of (.*)/\1\t\3\t\2/; p; s/.*/TOTAL/; x}' | sort > $(BUILD)/filecov.tsv
	mv *.gcov $(BUILDSUB)
	gcov -f -o $(BUILDSUB) -r -m $(CPPS) | sed -nEe '/^Function/ {s/^Function .(.*)./\1/; h}; /^Lines/ {H; x; s/(.*)\nLines executed:(.*) of (.*)/\1\t\3\t\2/; p; s/.*/unknown/; x}' | grep '^zezax::' | sort > $(BUILD)/funccov.tsv
	$(RM) *.gcov

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $?

$(SO): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $? -o $@

ifneq ($(wildcard $(BUILDSUB)/*.d), )
  include $(BUILDSUB)/*.d
endif
