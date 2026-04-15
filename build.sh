# Placeholder for build script
# Build dependencies: SDL2, GLEW, glm, compatible OpenGL drivers, iostream and cmath
# I will move to CMake eventually when the project isn't just 3 files
clang++ main.cpp -o cube -lSDL2 -lGLEW -lGL -ldl
