CXX = g++
CXXFLAGS = -std=c++17 -g -Wall -Wextra -Iinclude

TARGET = jitgrep
SRCDIR = src
OBJDIR = obj

SRCS = $(SRCDIR)/main.cpp $(SRCDIR)/jit.cpp $(SRCDIR)/regex_parser.cpp
OBJS = $(SRCS:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)
