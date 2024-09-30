SHELL := /bin/bash

# Application name, used in the target
APP_NAME := envrun

# Check if g++ is installed
CXX := g++
ifeq (, $(shell which $(CXX)))
$(error "No $(CXX) in PATH, consider installing g++ or changing the CXX variable")
endif

RM := rm -f
CXXFLAGS := -g -Wall -Wextra -std=c++20
LDFLAGS := -g
LDLIBS :=

# Platform specific variables
EXEC_EXT := $(if $(findstring Windows_NT,$(OS)),.exe,)

# Destination
OUT_DIR := ./bin
APP_EXE := $(OUT_DIR)/$(APP_NAME)$(EXEC_EXT)

# Build dir
BUILD_DIR := ./build

# Include directories
INCLUDE_DIRS := ./include

# Ignore directories temp/ notes/ and hidden directories
IGNORE_DIRS := temp notes

# A literal space.
space :=
space +=

# Joins elements of the list in arg 2 with the given separator.
#   1. Element separator.
#   2. The list.
join-with = $(subst $(eval ) ,$1,$(wildcard $2))

IGNORE_DIRS_F := -path $(call join-with, -o -path ,$(IGNORE_DIRS))

# Source files
SRCS := $(shell find . \( $(IGNORE_DIRS_F) \) -prune -o -type f -name '*.cpp')

# Object files
OBJS := $(patsubst ./%, $(BUILD_DIR)/%, $(SRCS:%.cpp=%.o))

# Dependencies
DEPS := $(OBJS:%.o=%.d)	

.DEFAULT_GOAL := $(APP_EXE)

.PHONY: all brun run info clean distclean $(APP_EXE)

all: $(APP_EXE)

# Link the object files
$(APP_EXE): $(OBJS)
	@mkdir -p $(OUT_DIR)
	$(CXX) $(LDFLAGS) $^ $(LDLIBS) -o $@

# Compile the source files
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@ $(addprefix -I, $(INCLUDE_DIRS))

# Build and run the application
brun: $(APP_EXE)
	$(APP_EXE)

# Run the application
run:
	$(APP_EXE)

info:
	@echo "APP_NAME: $(APP_NAME)"
	@echo "APP_EXE: $(APP_EXE)"
	@echo "IGNORE_DIRS: $(IGNORE_DIRS)"
	@echo "INCLUDE_DIRS: $(INCLUDE_DIRS)"
	@echo "SRCS: $(SRCS)"
	@echo "OBJS: $(OBJS)"
	@echo "DEPS: $(DEPS)"

clean:
	$(RM) $(OBJS) $(DEPS)

distclean: clean
	$(RM) $(APP_EXE)

include $(wildcard $(DEPS))
