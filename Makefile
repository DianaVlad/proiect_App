build: ser serial_opt omp mpi_omp

ser:
	gcc -Wall -g serial/matrix/riib_int.c -o ser

serial_opt:
	gcc -Wall -g serial/vector/riib_int.c -o serial_opt

omp:
	cc -Wall  -g openmp/openmp.c -o omp -fopenmp -lm

mpi_omp: 
	mpicc -Wall -g hibrid/mpi_omp.c -o mpi_omp -fopenmp -lm

clean:
	rm -f ser serial_opt omp mpi_omp
