CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
TARGET = rain

all: $(TARGET)

$(TARGET): rain.cpp
	$(CXX) $(CXXFLAGS) rain.cpp -o $(TARGET)

clean:
	rm -f $(TARGET)