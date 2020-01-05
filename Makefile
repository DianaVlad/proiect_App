build: ser serial_opt omp riib_mpi mpi_omp

ser:
	gcc -Wall -g serial/matrix/riib_float.c -o ser

serial_opt:
	gcc -Wall -g serial/vector/riib_int.c -o serial_opt

omp:
	gcc -Wall  -g openmp/openmp.c -o omp -fopenmp -lm

riib_mpi: 
	mpicc -Wall -g mpi/riib_mpi_iv.c -o riib_mpi_iv -lm

mpi_omp: 
	mpicc -Wall -g hibrid/mpi_omp.c -o mpi_omp -fopenmp -lm

clean:
	rm -f ser serial_opt omp riib_mpi mpi_omp
