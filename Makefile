CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17
TARGET = bin/rain
DOCKER_IMAGE_NAME = rain-app

.PHONY: all clean docker-build docker-run help

all: $(TARGET)

$(TARGET): rain.cpp
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) rain.cpp -o $(TARGET)

clean:
	rm -rf bin

docker-build:
	@echo "building Docker image..."
	@docker build -t $(DOCKER_IMAGE_NAME) .

docker-run: docker-build
	@echo "running application in Docker..."
	@docker run -it --rm $(DOCKER_IMAGE_NAME) $(ARGS)

help:
	doccer-build  - build the docker image
	docker-run    - run the application in a docker container
	help          - show this help message

	to pass arguments to docker-run, use the args variable:
	  make docker-run args="--lightning --lightning-duration 1"
