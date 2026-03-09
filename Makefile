CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lglew -lglfw -framework OpenGL

SOURCES = main.cpp \
          geometry/room_geometry.cpp \
          geometry/room2_geometry.cpp \
          geometry/tree_geometry.cpp \
          geometry/curtain_geometry.cpp \
          geometry/window_geometry.cpp \
          geometry/mouse_geometry.cpp \
          geometry/picture_frame_geometry.cpp \
          geometry/forest_tree_geometry.cpp \
          geometry/grandfather_clock_geometry.cpp

OBJECTS = $(SOURCES:.cpp=.o)

TARGET = finalproject

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
