CC := gcc
INCLUDES := ./include
SRCDIR := src
BUILDDIR := build

CFLAGS := -Wall -Wextra -I$(INCLUDES)
LDLIBS := -lpthread

TARGET := IRCserver

SRCS := $(wildcard $(SRCDIR)/*.c)
OBJS := $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SRCS))

all: $(BUILDDIR) $(TARGET)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDLIBS)

$(INCLUDES)/pch.h.gch: $(INCLUDES)/pch.h
	$(CC) $(CFLAGS) -x c-header $< -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(INCLUDES)/pch.h.gch
	$(CC) $(CFLAGS) -include pch.h -c $< -o $@

clean:
	rm -rf $(BUILDDIR)

fclean: clean
	rm -f $(TARGET)

re: fclean all

.PHONY: all clean fclean re
