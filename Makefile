all: cnfmiter atleasttwosolutions

cnfmiter: Main.cc Dimacs.h ParseUtils.h SolverTypes.h Makefile
	g++ Main.cc -o cnfmiter -std=c++11 -lz

atleasttwosolutions: AtLeastTwoSolutions.cc Dimacs.h ParseUtils.h SolverTypes.h Makefile
	g++ AtLeastTwoSolutions.cc -o atleasttwosolutions -std=c++11 -lz


clean:
	rm -f cnfmiter
	rm -f atleasttwosolutions
