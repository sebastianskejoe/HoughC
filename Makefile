all:
	gcc -g -Wall -pthread stb_image.c main.c -lGL -lGLU -lglut -lm
