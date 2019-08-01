all: ps

install:
	sudo apt-get install libglu1-mesa-dev freeglut3-dev mesa-common-dev
clean:
	rm *.o
project_space_gl.o: project_space_gl.cpp
	g++ -c -o project_space_gl.o project_space_gl.cpp
ps: project_space_gl.o
	g++ -o ps project_space_gl.o -lglut -lGLU -lGL