//########################################################################
//## Copyright 2019 Da Yan http://www.cs.uab.edu/yanda
//##
//## Licensed under the Apache License, Version 2.0 (the "License");
//## you may not use this file except in compliance with the License.
//## You may obtain a copy of the License at
//##
//## //http://www.apache.org/licenses/LICENSE-2.0
//##
//## Unless required by applicable law or agreed to in writing, software
//## distributed under the License is distributed on an "AS IS" BASIS,
//## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//## See the License for the specific language governing permissions and
//## limitations under the License.
//########################################################################

//########################################################################
//## Contributors
//##
//##
//########################################################################

#include "kernel_app.h"
#include <math.h> // used for ceil method, as it won't compile without it on Windows. 
#include <signal.h>
#include <iomanip>
// #include <chrono>
using namespace std::chrono;

void request_graceful_stop(int)
{
	global_stop_requested = true;
}

void request_emergency_cleanup_and_exit(int signal_no)
{
	emergency_cleanup_and_exit(signal_no);
}

int main(int argc, char **argv)
{
	try
	{
	signal(SIGINT, request_graceful_stop);
	signal(SIGTERM, request_graceful_stop);
	signal(SIGHUP, request_graceful_stop);
	signal(SIGABRT, request_emergency_cleanup_and_exit);
	signal(SIGSEGV, request_emergency_cleanup_and_exit);
	signal(SIGFPE, request_emergency_cleanup_and_exit);
	signal(SIGILL, request_emergency_cleanup_and_exit);
#ifdef SIGBUS
	signal(SIGBUS, request_emergency_cleanup_and_exit);
#endif

    auto program_start = steady_clock::now();

    if(argc != 8 && argc != 9){
		cout<<"arg1 = input path, arg2 = number of threads"
				<<", arg3 = degree ratio, arg4 = min_size, arg5 = time delay threshold, "
						<<"arg6 = kernel file, arg7 = prime_k, arg8 = use trie check"<<endl;
		return -1;
	}
    char* input_path = argv[1];
    num_compers = atoi(argv[2]); //number of compers
	gdmin_deg_ratio = atof(argv[3]);
	gnmin_size = atoi(argv[4]);
	gnmax_size = INT_MAX;
	gnmin_deg = ceil(gdmin_deg_ratio * (gnmin_size - 1));
	TIME_THRESHOLD = atof(argv[5]);
	configure_thread_output_target(input_path, argv[3]);
	kernel_file = argv[6];
	prime_k = atoi(argv[7]);
	if(argc == 9)
		trie_check = atoi(argv[8]);
	cout << "THREAD_OUTPUT_TEMPLATE:" << build_thread_output_file_path(0) << endl;


    QCWorker worker(num_compers);
	auto start = steady_clock::now();
    worker.load_data(input_path);
    auto end = steady_clock::now();
    double load_data_sec = duration<double>(end - start).count();
    // worker.initialize_tasks();
    start = steady_clock::now();
    worker.run();
    end = steady_clock::now();
    double run_total_sec = duration<double>(end - start).count();

    cout << fixed << setprecision(6);
    cout << "load_data() execution time:" << load_data_sec << endl;
    cout << "run() execution time:" << run_total_sec << endl;
    cout << "Execution Time:" << duration<double>(steady_clock::now() - program_start).count() << endl;

    cout << "trie count: " << trie->print_result() << endl;

    log("Done");
	return 0;
	}
	catch (const std::exception &e)
	{
		cerr << "Fatal error: " << e.what() << endl;
		return 1;
	}
	catch (...)
	{
		cerr << "Fatal error: unknown exception" << endl;
		return 1;
	}
}

//./run ...
