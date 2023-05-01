# flume Makefile

CFLAGS = -pthread -w -Wall -Wshadow -Wpointer-arith -Wwrite-strings

CXX := g++
CXXFLAGS = $(CFLAGS) -Wsign-promo
CXXDEPEND := -MMD
CXXDEPENDONLY := -MM
LDLIBS := -lrrd
CPPFLAGS :=

ifeq ($(MODE), opt)
  CFLAGS += -O2
else ifeq ($(MODE), asan)
  CFLAGS += -g -fno-omit-frame-pointer -fsanitize=address
else ifeq ($(MODE), jem)
  CFLAGS += -g -fno-omit-frame-pointer -DUSE_JEMALLOC
  LDLIBS += -ljemalloc
else
  CFLAGS += -g -fno-omit-frame-pointer
endif

EXE := flume
CPPS := $(wildcard *.cpp)

.PHONY: all clean depend

all: flume

%.o: %.cpp
	$(CXX) -c $(CXXDEPEND) $(CXXFLAGS) $(CPPFLAGS) $< -o $@

%.d: %.cpp
	$(CXX) $(CXXDEPENDONLY) $(CPPFLAGS) $< > $@

$(EXE): $(CPPS:.cpp=.o)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	$(RM) flume *.o *.d

depend: $(CPPS:.cpp=.d)

ifneq ($(wildcard *.d), )
  include *.d
endif
