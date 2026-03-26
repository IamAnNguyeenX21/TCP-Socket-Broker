CC = gcc
CFLAGS = -Wall -Wextra -pthread
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
all: app_multi_thread
app_multi_thread: $(OBJS)
	$(CC) $(CFLAGS) -o app_multi_thread $(OBJS)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:	
	rm -f *.o *.elf app_multi_thread