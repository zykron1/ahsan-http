# Compiler and flags
CC = gcc
CFLAGS = -Wall -fPIC
LDFLAGS = -L. -lahsan_http

# Target names
LIB_NAME_STATIC = libahsan_http.a
PROGRAM_NAME = example

# Source files
LIB_SRC = ahsan_http.c
PROGRAM_SRC = example.c

# Object files
LIB_OBJ = $(LIB_SRC:.c=.o)
PROGRAM_OBJ = $(PROGRAM_SRC:.c=.o)

# Default target
all: $(LIB_NAME_STATIC) $(PROGRAM_NAME)

# Rule to build static library
$(LIB_NAME_STATIC): $(LIB_OBJ)
	ar rcs $@ $(LIB_OBJ)

# Rule to build the example program using the static library
$(PROGRAM_NAME): $(PROGRAM_SRC) $(LIB_NAME_STATIC)
	$(CC) -o $@ $(PROGRAM_SRC) $(LDFLAGS)

# Clean up generated files
clean:
	rm -f *.o $(LIB_NAME_STATIC) $(PROGRAM_NAME)

