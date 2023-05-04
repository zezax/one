# Makefile

OPT := -g -fsanitize=address
LDLIBS += -lasan

WARN := -Wall -Wextra -Wshadow -Wpointer-arith -Wwrite-strings -Wsign-promo \
-Wuninitialized -Wconversion -Wfloat-equal -Wrestrict \
-Wdisabled-optimization -Wduplicated-branches -Wduplicated-cond -Wformat=2 \
-Wlogical-op -Wnull-dereference -Woverloaded-virtual -Wuseless-cast \
-Wimplicit-fallthrough=2 -Wmissing-include-dirs -Werror

CXX := g++
CXXFLAGS := -std=c++17 $(OPT) $(WARN)

LDLIBS += -lsensors

EXE := sensorrd

.PHONY: all clean

all: $(EXE)

clean:
	rm -f *.o $(EXE)

sensorrd: sensorrd.o wrapper.o
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $< -o $@
