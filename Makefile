CXX = g++
CXXFLAGS = -std=c++20
LDFLAGS = -ldpp -lsqlite3
SRC = $(wildcard *.cpp)
OBJ = $(SRC:.cpp=.o)  # Convert .cpp files to .o object files
TARGET = bot

# Default rule
all: $(TARGET)

# Linking step
$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

# Compilation step: create object files from source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up object files and the final executable
clean:
	rm -f $(TARGET) $(OBJ)
