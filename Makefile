
# make –> Builds only the shared library (if LIB_TYPE is dynamic) because no USERAPPFILEPATH is provided.
# make LIB_TYPE=static –> Builds only the static library.
# make USERAPPFILEPATH=./RECIVING_FILE_APP/file_sender_server.c –> Builds the shared library (dynamic) and links the provided C file into an executable named.
# make LIB_TYPE=static USERAPPFILEPATH=./RECIVING_FILE_APP/file_sender_server.c –> Builds the static library and links the provided C file into an executable named file_sender_server.
# make clean –> Removes all object files, the library file (either .so or .a), and the executable.

.PHONY: help
help:
	@echo "Usage: make [target] [VARIABLE=value]"
	@echo ""
	@echo "Targets:"
	@echo "  all               Build the library (and executable if USERAPPFILEPATH is provided)"
	@echo "  headers           Copy header files"
	@echo "  clean             Remove all generated files (object files, libraries, executables)"
	@echo "  help              Show this help text"
	@echo ""
	@echo "Options:"
	@echo "  LIB_TYPE=static   Build static library (default is dynamic)"
	@echo "  USERAPPFILEPATH=./path/to/app.c"
	@echo "                    If provided, links the C file into an executable"


.PHONY: all headers clean
.DEFAULT_GOAL := all

CC = gcc

MYLIB_INCLUDE_DIR := ./mylib/include
CLIENT_HEADERS := $(wildcard ./CLIENT_SERVER/include/*.h) $(wildcard ./CLIENTS_MANAGER/include/*.h) $(wildcard ./BUDDY_ALLOCATOR/include/*.h) $(wildcard ./STORAGE_MANAGER/include/*.h)
INFRA_HEADERS := $(wildcard ./INFRA/include/*.h)

CFLAGS = -Wall -fPIC -I$(MYLIB_INCLUDE_DIR) -I./FILE_TRANSFER_INFRA/ -I./include
LDFLAGS = -L./mylib -Wl,-rpath=\$$ORIGIN/../mylib
LIBS = -lfile_transfer_infra

# Directory for executables
BINDIR := bin

# Choose library type: set LIB_TYPE=static to use the static library.
# Default is dynamic.
LIB_TYPE ?= dynamic


ALL_DIRS := ./INFRA/src ./CLIENT_SERVER/src ./CLIENTS_MANAGER/src ./BUDDY_ALLOCATOR/src ./STORAGE_MANAGER/src
SRC := $(foreach dir,$(ALL_DIRS),$(wildcard $(dir)/*.c))

OBJ = $(SRC:.c=.o)

SO_NAME = ./mylib/libfile_transfer_infra.so
A_NAME = ./mylib/libfile_transfer_infra.a

# USERAPPFILEPATH is empty by default.
USERAPPFILEPATH ?=

# Copy header files target
headers:
	@mkdir -p $(MYLIB_INCLUDE_DIR)
	cp $(CLIENT_HEADERS) $(MYLIB_INCLUDE_DIR)
	cp $(INFRA_HEADERS) $(MYLIB_INCLUDE_DIR)

	
# Set library variables based on chosen type
ifeq ($(LIB_TYPE),static)
  LIB = $(A_NAME)
  LIB_LINK = $(A_NAME)
else
  LIB = $(SO_NAME)
  LIB_LINK = -lfile_transfer_infra
endif


# If a user application file is provided, derive the executable name.
ifeq ($(strip $(USERAPPFILEPATH)),)
  APP_TARGET :=
else
  USER_APP_FILE = ./$(USERAPPFILEPATH)
  APP_NAME = $(basename $(notdir $(USER_APP_FILE)))
  APP_TARGET := $(BINDIR)/$(APP_NAME)
endif

# Default target: build the library and, if applicable, the executable.
all: headers $(LIB) $(APP_TARGET)

# Rule for compiling .c files to .o object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Library rules:
ifeq ($(LIB_TYPE),static)
$(A_NAME): $(OBJ)
	@if [ -f $@ ]; then \
	  echo "$@ exists, skipping creation"; \
	else \
	  ar rcs $@ $^; \
	fi
else
$(SO_NAME): $(OBJ)
	@if [ -f $@ ]; then \
	  echo "$@ exists, skipping creation"; \
	else \
	  $(CC) -shared -o $@ $^; \
	fi
endif

$(BINDIR):
	@mkdir -p $(BINDIR)

# Define the executable target only if USERAPPFILEPATH is provided.
ifeq ($(strip $(USERAPPFILEPATH)),)
# No executable rule if no file is provided.
else
$(BINDIR)/$(APP_NAME): $(USER_APP_FILE) $(LIB) | $(BINDIR)

ifeq ($(LIB_TYPE),static)
	$(CC) $(CFLAGS) -o $(BINDIR)/$(APP_NAME) $(USER_APP_FILE) $(LIB_LINK)
else
	$(CC) $(CFLAGS) -o $(BINDIR)/$(APP_NAME) $(USER_APP_FILE) $(LDFLAGS) $(LIB_LINK)
endif
endif

# Clean up generated files.
clean:
	rm -f $(OBJ) $(SO_NAME) $(A_NAME) $(APP_TARGET)
	rm -rf bin/*
	rm -rf $(MYLIB_INCLUDE_DIR)/*.h
