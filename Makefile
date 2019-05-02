SRCDIR   = src
BINDIR   = bin
INCLUDES = include

#CC=gcc
CXX=g++
CFLAGS=-Wall -Wextra -g -fno-stack-protector -z execstack -pthread -std=c++11 -I $(INCLUDES)/ -m32
DEPS = $(wildcard $(INCLUDES)/%.h)

all: $(BINDIR)/client $(BINDIR)/server $(DEPS)

$(BINDIR):
	mkdir -p $(BINDIR)

$(BINDIR)/client: $(SRCDIR)/client.cpp $(BINDIR)
	$(CXX) $(CFLAGS) $< -o $@

$(BINDIR)/server: $(SRCDIR)/server.cpp $(BINDIR)
	$(CXX) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	rm -f $(BINDIR)/client $(BINDIR)/server
