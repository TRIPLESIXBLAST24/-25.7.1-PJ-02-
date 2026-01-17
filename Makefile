CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
TARGET = chat_app
SOURCES = main.cpp chat.cpp server.cpp database.cpp message.cpp user.cpp
OBJECTS = $(SOURCES:.cpp=.o)
HEADERS = chat.h server.h database.h message.h user.h

# Определяем операционную систему
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LDFLAGS = -pthread
endif
ifeq ($(UNAME_S),Darwin)
    LDFLAGS = 
endif

# Windows
ifeq ($(OS),Windows_NT)
    LDFLAGS = -lws2_32
endif

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean

