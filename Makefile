CC = gcc
CFLAGS = -Wall -O2 -std=gnu99 `pkg-config --cflags gtk+-3.0 libarchive libcurl openssl`
LDFLAGS = `pkg-config --libs gtk+-3.0 libarchive libcurl openssl` -ludev -lpthread
TARGET = lufus
SRCS = lufus.c dev.c format.c iso.c badblocks.c hash.c net.c ui.c ddwrite.c wim.c linux2win.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
