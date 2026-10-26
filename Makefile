CXX ?= g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O3 -Iinclude -MMD -MP

ifeq ($(OS),Windows_NT)
    TARGET = $(BIN_DIR)/religion_sim.exe
    TEST_TARGET = $(BIN_DIR)/test_simulation.exe
    LDFLAGS = -lopengl32 -lgdi32 -luser32 -lpthread
else
    TARGET = $(BIN_DIR)/religion_sim
    TEST_TARGET = $(BIN_DIR)/test_simulation
    LDFLAGS = -lX11 -lGL -lpthread
endif

SRC_DIR = src
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin
TEST_DIR = tests

SRCS = $(SRC_DIR)/IsometricCamera.cpp \
       $(SRC_DIR)/Religion.cpp \
       $(SRC_DIR)/Cell.cpp \
       $(SRC_DIR)/MultiNoiseBiome.cpp \
       $(SRC_DIR)/WorldMap.cpp \
       $(SRC_DIR)/AdvancementSystem.cpp \
       $(SRC_DIR)/ReplicationEngine.cpp \
       $(SRC_DIR)/AgglutinationEngine.cpp \
       $(SRC_DIR)/Villager.cpp \
       $(SRC_DIR)/TileSet.cpp \
       $(SRC_DIR)/Humano.cpp \
       $(SRC_DIR)/Timeline.cpp \
       $(SRC_DIR)/TimeEngine.cpp \
       $(SRC_DIR)/EventLine.cpp \
       $(SRC_DIR)/Renderer.cpp \
       $(SRC_DIR)/UISystem.cpp \
       $(SRC_DIR)/SaveSystem.cpp \
       $(SRC_DIR)/Game.cpp

OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

TARGET = $(BIN_DIR)/religion_sim
TEST_TARGET = $(BIN_DIR)/test_simulation

.PHONY: all clean run test

all: $(TARGET)

$(TARGET): $(BUILD_DIR)/main.o $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Built target: $@"

$(TEST_TARGET): $(BUILD_DIR)/test_simulation.o $(OBJS) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "Built test target: $@"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test_simulation.o: $(TEST_DIR)/test_simulation.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) *.ppm

-include $(wildcard $(BUILD_DIR)/*.d)
