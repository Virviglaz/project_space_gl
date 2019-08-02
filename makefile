all: ps

install:
	sudo apt-get install libglu1-mesa-dev freeglut3-dev mesa-common-dev
clean:
	rm *.o
engine.o: engine.c
	gcc -c -o engine.o engine.c
project_space_gl.o: project_space_gl.c
	gcc -c -o project_space_gl.o project_space_gl.c
ps: project_space_gl.o engine.o
	gcc -o ps project_space_gl.o engine.o -lglut -lGLU -lGL -lpthread