CXX := g++
#CXXFLAGS := -ggdb3
CXXFLAGS := -O3
CXXFLAGS += -w -Wall -Wno-parentheses -Wshadow -Wpointer-arith -Wwrite-strings -Wsign-promo -felide-constructors -fno-operator-names

EXE := loggle

all: $(EXE)

clean:
	rm -f $(EXE) *.o core

$(EXE): loggle.o
	$(CXX) $^ -lpthread -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $^ -o $@
