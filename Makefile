cli:
	gcc -std=c99 cli.c -fopenmp -lm -w -march=native -O3 -o jpeg

test:
	gcc -std=c99 test.c -fopenmp -lm -w -march=native -O3 -o test
	./test
	rm test lena.jpg

clean:
	rm test jpeg lena.jpg