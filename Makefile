# ============================================================================
# Makefile — build & run doctest for the Tividy document system
#
# Targets:
#   make         build ./test_contract
#   make run     build then run the tests
#   make clean   remove build artifacts
#   make rebuild clean + build + run
#   make help    show this message
# ============================================================================

# Disable Make's builtin rules. Without this, an unknown target like
# "ActiveState" falls back to "compile ActiveState.cpp into a binary" and
# produces confusing linker errors instead of telling you a file is missing.
.SUFFIXES:
MAKEFLAGS += --no-builtin-rules

# ---- Toolchain -----------------------------------------------------------
CXX      ?= g++
CXXFLAGS ?= -std=c++11 -Wall -Wextra -g -O0
CPPFLAGS ?=
LDFLAGS  ?=
LDLIBS   ?= -lncurses

# ---- Layout --------------------------------------------------------------
TEST_SRC  := tests.cpp
BIN       := test_contract
DOCTEST_H := doctest.h
INCLUDES  := -I.

# ---- Sources -------------------------------------------------------------
# Every .cpp in this directory except:
#   - the test TU itself
#   - main.cpp                  (has its own main())
#   - generic_driver_reference.cpp (has its own main())
# Add any other TUs that must not be linked into the test binary to EXCLUDE.
EXCLUDE   := $(TEST_SRC) main.cpp generic_driver_reference.cpp
PROD_SRCS := $(filter-out $(EXCLUDE), $(wildcard *.cpp))

OBJDIR    := build
PROD_OBJS := $(patsubst %.cpp,$(OBJDIR)/%.o,$(PROD_SRCS))
TEST_OBJ  := $(OBJDIR)/$(TEST_SRC:.cpp=.o)

# ---- Default -------------------------------------------------------------
.PHONY: all
all: $(BIN)

# ---- Link ----------------------------------------------------------------
$(BIN): $(TEST_OBJ) $(PROD_OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

# ---- Compile -------------------------------------------------------------
$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

# The test object additionally depends on doctest.h so editing it triggers
# a rebuild.
$(TEST_OBJ): $(TEST_SRC) $(DOCTEST_H)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) $(INCLUDES) -c $< -o $@

# ---- Convenience ---------------------------------------------------------
.PHONY: run
run: $(BIN)
	./$(BIN)

.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(BIN)

.PHONY: rebuild
rebuild: clean run

.PHONY: help
help:
	@echo "make          - build $(BIN)"
	@echo "make run      - build and run the tests"
	@echo "make clean    - remove build artifacts"
	@echo "make rebuild  - clean, build, run"
	@echo "make help     - show this message"