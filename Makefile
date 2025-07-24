CC := gcc
CFLAGS := -Wall -Wextra -Iinclude -ggdb

INC_DIR := include
SRC_DIR := src
OBJ_DIR := obj

INCS := $(wildcard $(INC_DIR)/*.h)
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))

TARGET := pudel

all: $(TARGET)

$(TARGET): $(OBJ_DIR)/pudel.o $(OBJS) $(INCS)
	$(CC) $(CFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INCS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/pudel.o: pudel.c $(INCS) | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -fr $(OBJ_DIR) $(TARGET)

.PHONY: all clean
