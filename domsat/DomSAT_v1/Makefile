Dist: wscp.h main.cpp
	g++ \
	-fdiagnostics-color=always \
	-I/home/wyy/ibm/ILOG/CPLEX_Studio2211/cplex/include \
	-I/home/wyy/ibm/ILOG/CPLEX_Studio2211/concert/include \
	-DIL_STD \
	main.cpp \
	-L/home/wyy/ibm/ILOG/CPLEX_Studio2211/cplex/lib/x86-64_linux/static_pic \
	-L/home/wyy/ibm/ILOG/CPLEX_Studio2211/concert/lib/x86-64_linux/static_pic \
	-lconcert -lilocplex -lcplex -lm -lpthread \
	-O3 -o DomSAT

	rm -f *~
