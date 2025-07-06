CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude

SRCDIR = src
INCDIR = include
OBJDIR = obj
DOCKERDIR = docker

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Create object directory if it doesn't exist
$(shell mkdir -p $(OBJDIR))

build:
	docker compose -f $(DOCKERDIR)/docker-compose.yml build

up:
	docker compose -f $(DOCKERDIR)/docker-compose.yml up -d

down:
	docker compose -f $(DOCKERDIR)/docker-compose.yml down

exec:
	docker compose -f $(DOCKERDIR)/docker-compose.yml exec dev /bin/bash

restart:
	docker compose -f $(DOCKERDIR)/docker-compose.yml down
	docker compose -f $(DOCKERDIR)/docker-compose.yml up -d

ft_ping: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lm

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -MMD -c $< -o $@

-include $(OBJDIR)/*.d

clean:
	rm -rf $(OBJDIR) ft_ping

ping-run: ping
	./ping google.com

.PHONY: build up down exec restart clean ping-run
