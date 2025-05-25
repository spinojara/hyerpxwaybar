CC      = cc
CFLAGS  = -Wall -Wextra -Wshadow -pedantic -O2 -std=c99 $(pkg-config --cflags hidapi-hidraw)
LDFLAGS = $(CFLAGS)
LDLIBS  = $(shell pkg-config --libs hidapi-hidraw)

hyerpxwaybar: hyerpxwaybar.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
