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
	gcov -o $(BUILDSUB) -r -m $(CPPS) | sed -nEe '/^File/ {s/^File .(.*)./\1/; s:../include/::; h}; /^Lines/ {H; x; s/(.*)\nLines executed:(.*)% of (.*)/\2\t\3\t\1/; p; s/.*/TOTAL/; x}' | sort -k1,1n -k2,2nr -k3,3 > $(BUILD)/filecov.tsv
	mv *.gcov $(BUILDSUB)
	gcov -f -o $(BUILDSUB) -r -m $(CPPS) | sed -nEe '/^Function/ {s/^Function .(.*)./\1/; h}; /^Lines/ {H; x; s/(.*)\nLines executed:(.*)% of (.*)/\2\t\3\t\1/; p; s/.*/unknown/; x}' | sed -ne '/\tzezax::red::/p' | sed -e 's/zezax::red:://g' | sort | sort -k1,1n -k2,2nr -k3,3 > $(BUILD)/funccov.tsv
	$(RM) *.gcov

tidy: $(CPPS:%.cpp=tidy-%)

tidy-%: %.cpp
	-clang-tidy $^ -- --std=c++20 -Wall -Wextra -iquote $(ROOT)/include

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $?

$(SO): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $? -o $@

ifneq ($(wildcard $(BUILDSUB)/*.d), )
  include $(BUILDSUB)/*.d
endif
