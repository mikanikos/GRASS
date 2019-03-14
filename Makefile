SRCDIR   = src
BINDIR   = bin
INCLUDES = include

#CC=gcc
CXX=g++
CFLAGS=-Wall -Wextra -g -fno-stack-protector -z execstack -lpthread -std=c++11 -I $(INCLUDES)/ -m32
DEPS = $(wildcard $(INCLUDES)/%.h)

all: $(BINDIR)/client $(BINDIR)/server $(DEPS)

$(BINDIR)/client: $(SRCDIR)/client.cpp
	$(CC) $(CFLAGS) $< -o $@

$(BINDIR)/server: $(SRCDIR)/server.cpp
	$(CC) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(BINDIR)/client $(BINDIR)/server
