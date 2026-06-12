CC      = gcc
CFLAGS ?= -O2 -Wall -Wextra
LDFLAGS  = -shared -fPIC
LDLIBS   = -ldl

TARGET   = setmark.so
SRC      = setmark.c

TEST_SRC = tests/test_mark.c
TEST_BIN = tests/test_mark

PREFIX  ?= /usr/local
LIBDIR  ?= $(PREFIX)/lib

.PHONY: all clean test test-bin install uninstall

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

test-bin: $(TEST_BIN)

$(TEST_BIN): $(TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $<

test: $(TARGET) $(TEST_BIN)
	@bash tests/run_tests.sh

install: $(TARGET)
	install -D -m 755 $(TARGET) $(DESTDIR)$(LIBDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/$(TARGET)

clean:
	rm -f $(TARGET) $(TEST_BIN)
