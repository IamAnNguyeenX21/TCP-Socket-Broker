CC = gcc
CFLAGS = -Wall -Wextra -pthread
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
all: app
app: $(OBJS)
	$(CC) $(CFLAGS) -o app routing_table.c main.c

client:
	$(CC) $(CFLAGS) -o client client.c
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:	
	rm -f *.o *.elf app client