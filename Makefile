cli:
	gcc-6 -std=c99 cli.c -fopenmp -lm -w -march=native -O3 -o jpeg

cli-slow:
	gcc-6 -std=c99 cli.c -fopenmp -lm -w -march=native -O3 -D UNOPTIMIZED -o jpeg

test:
	gcc-6 -std=c99 test.c -fopenmp -lm -w -march=native -O3 -o test
	./test
	rm test lena.jpg

test-slow:
	gcc-6 -std=c99 test.c -fopenmp -lm -w -march=native -O3 -D UNOPTIMIZED -o test
	./test
	rm test lena.jpg

clean:
	rm test jpeg lena.jpg