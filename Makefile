CC = gcc
CFLAGS = -O2
LDFLAGS = -rdynamic
DEFS   = -DUSE_GNU_READL=1
LIBS   = -lreadline -lm
SRCDIR = src
TARGET = seal

SRCS = $(wildcard $(SRCDIR)/*.c)

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS) $(LIBS)

$(SRCDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(DEFS) -c $< -o $@

clean:
	rm -f $(SRCDIR)/*.o $(TARGET)

.PHONY: all clean
