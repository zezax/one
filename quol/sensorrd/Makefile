# Makefile

ifeq ($(MODE), opt)
  OPT := -O3 -march=native
else
  OPT := -g -fsanitize=address
  LDLIBS += -lasan
endif

WARN := -Wall -Wextra -Wshadow -Wpointer-arith -Wwrite-strings -Wsign-promo \
-Wuninitialized -Wconversion -Wfloat-equal -Wrestrict \
-Wdisabled-optimization -Wduplicated-branches -Wduplicated-cond -Wformat=2 \
-Wlogical-op -Wnull-dereference -Woverloaded-virtual -Wuseless-cast \
-Wimplicit-fallthrough=2 -Wmissing-include-dirs -Werror

CXX := g++
CXXFLAGS := -std=c++17 $(OPT) $(WARN)

LDLIBS += -lsensors -lrrd

EXE := sensorrd
CPPS := $(wildcard *.cpp)
OBJS := $(CPPS:%.cpp=%.o)

.PHONY: all clean

all: $(EXE)

clean:
	rm -f *.o $(EXE)

$(EXE): $(OBJS)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@
