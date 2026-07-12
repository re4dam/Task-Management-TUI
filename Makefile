CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O3
LDFLAGS = -lncurses

TARGET = tui_tasks
SRCS = src/main.cpp

all: $(TARGET)

$(TARGET): $(SRCS) src/task.hpp src/list.hpp src/storage.hpp src/tui.hpp
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
