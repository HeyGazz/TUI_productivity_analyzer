CC      = clang
CFLAGS  = -std=c99 -Wall -Wextra -Wpedantic -D_XOPEN_SOURCE=600 \
          -g -O2
LDFLAGS = -lncurses

TARGET  = productivity
SRCS    = main.c activity.c storage.c ui.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

# Dependencies
main.o:     main.c activity.h storage.h ui.h
activity.o: activity.c activity.h
storage.o:  storage.c storage.h activity.h
ui.o:       ui.c ui.h activity.h
