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

#include "qc_app.h"
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

    auto program_start = steady_clock::now();

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

    if (argc < 6)
    {
        cout << "arg1 = input path, arg2 = number of threads"
             << ", arg3 = degree ratio, arg4 = min_size, arg5 = time delay threshold" << endl;
        return -1;
    }
    char *input_path = argv[1];
    num_compers = atoi(argv[2]); //number of compers
    gdmin_deg_ratio = atof(argv[3]);
    gnmin_size = atoi(argv[4]);
    gnmax_size = INT_MAX;
    gnmin_deg = ceil(gdmin_deg_ratio * (gnmin_size - 1));
    TIME_THRESHOLD = atof(argv[5]); // tau_time
    configure_thread_output_target(input_path, argv[3]);
    if (argc > 6)
        BIGTASK_THRESHOLD = atof(argv[6]); // tau_split
    if (argc > 7)
        tasks_per_fetch_g = atoi(argv[7]);
    if (argc > 8)
        Qbig_capacity = atoi(argv[8]);
    if (argc > 9)
        Qreg_capacity = atoi(argv[9]);
    if (argc > 10)
        BT_TASKS_PER_FILE = atoi(argv[10]);
    if (argc > 11)
        MINI_BATCH_NUM = atoi(argv[11]);
    if (argc > 12)
        RT_TASKS_PER_FILE = atoi(argv[12]);
    if (argc > 13)
        BT_THRESHOLD_FOR_REFILL = atoi(argv[13]);
    if (argc > 14)
        RT_THRESHOLD_FOR_REFILL = atoi(argv[14]);

    cout << "input_path:" << argv[1] << endl;
    cout << "num_compers:" << num_compers << endl;
    cout << "gdmin_deg_ratio:" << gdmin_deg_ratio << endl;
    cout << "gnmin_size:" << gnmin_size << endl;
    cout << "TIME_THRESHOLD:" << TIME_THRESHOLD << endl;
    cout << "THREAD_OUTPUT_TEMPLATE:" << build_thread_output_file_path(0) << endl;
    cout << "BIGTASK_THRESHOLD:" << BIGTASK_THRESHOLD << endl;
    cout << "tasks_per_fetch_g:" << tasks_per_fetch_g << endl;
    cout << "Qbig_capacity:" << Qbig_capacity << endl;
    cout << "Qreg_capacity:" << Qreg_capacity << endl;
    cout << "BT_TASKS_PER_FILE:" << BT_TASKS_PER_FILE << endl;
    cout << "MINI_BATCH_NUM:" << MINI_BATCH_NUM << endl;
    cout << "RT_TASKS_PER_FILE:" << RT_TASKS_PER_FILE << endl;
    cout << "BT_THRESHOLD_FOR_REFILL:" << BT_THRESHOLD_FOR_REFILL << endl;
    cout << "RT_THRESHOLD_FOR_REFILL:" << RT_THRESHOLD_FOR_REFILL << endl;
    

    QCWorker worker(num_compers);

    auto start = steady_clock::now();
    worker.load_data(input_path);
    auto end = steady_clock::now();
    double load_data_sec = duration<double>(end - start).count();
    cout << fixed << setprecision(6);
    cout << "load_data() execution time:" << load_data_sec << endl;
    worker.latest_elapsed_time = (float)load_data_sec; // used to calculate step time inside the worker

    start = steady_clock::now();
    worker.run();
    end = steady_clock::now();
    double run_total_sec = duration<double>(end - start).count();
    double run_compute_sec = run_total_sec - worker.init_time;
    if (run_compute_sec < 0)
        run_compute_sec = 0;

    cout << "initialize_tasks() execution time:" << worker.init_time << endl;
    cout << "run() execution time (total):" << run_total_sec << endl;
    cout << "run() execution time (compute-only):" << run_compute_sec << endl;
    cout << "total execution time:" << duration<double>(steady_clock::now() - program_start).count() << endl;
    // cout << "trie count: " << trie->print_result() << endl;
    // cout << "================================================" << endl;

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
