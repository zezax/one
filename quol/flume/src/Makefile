# flume Makefile

ifeq ($(MODE), opt)
  OPT := -O2
else
  OPT := -g
endif

CXX := g++
CXXFLAGS := $(OPT) -Wall -W -Wshadow -Wpointer-arith -Wwrite-strings -Wsign-promo -fsanitize=address -fno-omit-frame-pointer
CXXDEPEND := -MMD
CXXDEPENDONLY := -MM
LDLIBS := -lrrd -lpthread

EXE := flume
CPPS := $(wildcard *.cpp)

.PHONY: all clean depend

all: flume

%.o: %.cpp
	$(CXX) -c $(CXXDEPEND) $(CXXFLAGS) $(CPPFLAGS) $< -o $@

%.d: %.cpp
	$(CXX) $(CXXDEPENDONLY) $(CPPFLAGS) $< > $@

$(EXE): $(CPPS:.cpp=.o)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(LDLIBS) $^ -o $@

clean:
	$(RM) flume *.o *.d

depend: $(CPPS:.cpp=.d)

ifneq ($(wildcard *.d), )
  include *.d
endif
