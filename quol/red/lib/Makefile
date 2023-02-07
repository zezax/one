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

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $?

$(SO): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -shared $? -o $@

ifneq ($(wildcard $(BUILDSUB)/*.d), )
  include $(BUILDSUB)/*.d
endif
