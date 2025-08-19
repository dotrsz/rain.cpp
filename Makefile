CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
TARGET = bin/rain

all: $(TARGET)

$(TARGET): rain.cpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) rain.cpp -o $(TARGET)

clean:
	rm -f $(TARGET)
