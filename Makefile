# VitalRoute build
#   C engine  : gcc -std=c11   (src/engine)
#   C++ layers: g++ -std=c++17 (src/model, src/modules, src/main.cpp)
#   Link      : g++
#
#   make        builds bin/vitalroute
#   make test   builds and runs the test suite
#   make clean  removes build products (saved records are left alone)

CC       := gcc
CXX      := g++
CFLAGS   := -std=c11 -Wall -Wextra -g
CXXFLAGS := -std=c++17 -Wall -Wextra -g
CPPFLAGS := -Isrc/engine -Isrc/model -Isrc/modules
LDLIBS   := -lm

ifeq ($(OS),Windows_NT)
EXE := .exe
endif

ENGINE_SRC := $(wildcard src/engine/*.c)
CXX_SRC    := $(wildcard src/model/*.cpp src/modules/*.cpp)
ENGINE_OBJ := $(ENGINE_SRC:src/%.c=build/%.o)
CXX_OBJ    := $(CXX_SRC:src/%.cpp=build/%.o)
MAIN_OBJ   := build/main.o

APP            := bin/vitalroute$(EXE)
TEST_ENGINE    := bin/test_engine$(EXE)
TEST_SCENARIOS := bin/test_scenarios$(EXE)
TEST_BINS      := $(if $(wildcard tests/test_engine.c),$(TEST_ENGINE)) \
                  $(if $(wildcard tests/test_scenarios.cpp),$(TEST_SCENARIOS))

.PHONY: all test test-bins clean

all: $(APP)

$(APP): $(ENGINE_OBJ) $(CXX_OBJ) $(MAIN_OBJ)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDLIBS)

build/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

build/%.o: src/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -MMD -MP -c $< -o $@

$(TEST_ENGINE): tests/test_engine.c $(ENGINE_OBJ)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDLIBS)

$(TEST_SCENARIOS): tests/test_scenarios.cpp $(ENGINE_OBJ) $(CXX_OBJ)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -o $@ $^ $(LDLIBS)

test-bins: $(TEST_BINS)

test: test-bins
	@if [ -f tests/run_tests.sh ]; then MAKE="$(MAKE)" sh tests/run_tests.sh; \
	else echo "No tests yet."; fi

clean:
	rm -rf build bin

-include $(ENGINE_OBJ:.o=.d) $(CXX_OBJ:.o=.d) $(MAIN_OBJ:.o=.d)
