CXX = g++
CXXFLAGS = -std=c++17 -I./include -I./external/cpp-httplib -I./external
# Add Conda environment paths if defined
ifneq ($(CONDA_PREFIX),)
    CXXFLAGS += -I$(CONDA_PREFIX)/include
    LDFLAGS += -L$(CONDA_PREFIX)/lib
endif

LDFLAGS += -lcurl

# Attempt to link Boost System/Thread if needed, or just standard libs
# Boost Beast is header only, but Asio might need system?
# Usually header-only for Beast.
# If link errors occur, we might need -lboost_system -lboost_thread

SRCS = src/main.cpp src/player.cpp src/monster.cpp src/item.cpp src/battle.cpp src/upgrade_advisor.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = osrscalc

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f src/*.o $(TARGET)

