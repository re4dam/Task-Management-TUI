CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O3
LDFLAGS = -lncurses

TARGET = dotui
SRCS = src/main.cpp

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

all: $(TARGET)

$(TARGET): $(SRCS) src/task.hpp src/list.hpp src/storage.hpp src/tui.hpp
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

.PHONY: all clean install uninstall
