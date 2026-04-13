CC = gcc
# CFLAGS = -Wall -Wextra -g
CFLAGS =
DEFS   = -DUSE_GNU_READL=1
LIBS   = -lreadline
SRCDIR = src
TARGET = seal

SRCS = $(wildcard $(SRCDIR)/*.c)

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(DEFS) -c $< -o $@

clean:
	rm -f $(SRCDIR)/*.o $(TARGET)

.PHONY: all clean
