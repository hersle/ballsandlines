all:
	cc ballsandlines.c -o ballsandlines -lm -lglfw -lGL -lGLU

clean:
	rm ballsandlines
