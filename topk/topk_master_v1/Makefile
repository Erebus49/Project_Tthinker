CXX := g++
CXXFLAGS := -std=gnu++11 -O3 -Wall -Wextra
TARGET := quasi_clique
SRCS := $(wildcard source/*.cpp)
OBJS := $(SRCS:.cpp=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

source/%.o: source/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
