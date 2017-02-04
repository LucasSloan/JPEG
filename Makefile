cli:
	gcc-6 -std=c99 cli.c -fopenmp -lm -w -march=native -O3 -o jpeg

cli-slow:
	gcc-6 -std=c99 cli.c -fopenmp -fopenacc -lm -w -march=native -O3 -D UNOPTIMIZED -o jpeg

test:
	gcc-6 -std=c99 test.c -fopenmp -lm -w -march=native -O3 -o test
	./test 100
	rm test lena.jpg

test-slow:
	gcc-6 -std=c99 test.c -fopenmp  -fopenacc -D UNOPTIMIZED -lm -w -march=native -O3 -o test
	./test 1
	rm test lena.jpg

test-pgi:
	pgcc -c99 -acc -openmp -D UNOPTIMIZED -Minfo -fast -O3 -o test test.c
	./test 1
	rm test lena.jpg

clean:
	rm test jpeg lena.jpg