cnfmiter: Main.cc Dimacs.h ParseUtils.h SolverTypes.h Makefile
	g++ Main.cc -o cnfmiter -std=c++11 -lz

clean:
	rm -f cnfmiter