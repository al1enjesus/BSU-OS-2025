CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -02
TARGET = fork_example
SRCS = src/fork_example.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
$(CXX) $(CXXFLAGS) -o $@ $<

run: $(TARGET)
./$(TARGET)

clean:
rm -f $(TARGET)

.PHONY: all run clean
