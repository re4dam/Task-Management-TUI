TARGET = dotui
BUILD_DIR = build

all: $(TARGET)

$(TARGET):
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. && $(MAKE)
	cp $(BUILD_DIR)/$(TARGET) .

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

.PHONY: all clean install uninstall
