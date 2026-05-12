all:
	g++ \
	-fdiagnostics-color=always \
	-I/home/wyy/ibm/ILOG/CPLEX_Studio2211/cplex/include \
	-I/home/wyy/ibm/ILOG/CPLEX_Studio2211/concert/include \
	-DIL_STD \
	blend.cpp \
	-L/home/wyy/ibm/ILOG/CPLEX_Studio2211/cplex/lib/x86-64_linux/static_pic \
	-L/home/wyy/ibm/ILOG/CPLEX_Studio2211/concert/lib/x86-64_linux/static_pic \
	-lconcert -lilocplex -lcplex -lm -lpthread \
	-O3 -o blend

# export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/home/wyy/ibm/ILOG/CPLEX_Studio2211/cplex/bin/x86-64_linux:/home/wyy/ibm/ILOG/CPLEX_Studio2211/concert/bin/x86-64_linux
# find /home -name "ilocplex.h" 2>/dev/null
