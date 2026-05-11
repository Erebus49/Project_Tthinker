#ifndef GLOBAL_H
#define GLOBAL_H

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <thread>
#include "conque.h"
#include <ext/hash_set>
#include <ext/hash_map>
#include <cstring>
#include <stack>
#include <bits/stdc++.h>
#include <dirent.h>
#include <unistd.h>
#include <cstdlib>
#include <cerrno>
#include <signal.h>
#include <sys/wait.h>
// If not Linux (e.g. Windows), include direct.h because it doesn't work in Linux.
#ifndef __linux__
#include <direct.h>
#endif

#define hash_map __gnu_cxx::hash_map // ??
#define hash_set __gnu_cxx::hash_set // ??
// Used for S_IRWXU
#include <sys/stat.h>

using namespace std;

// Number of idle compers
atomic<int> global_num_idle(0);
atomic<bool> global_end_label(false);
atomic<bool> global_stop_requested(false);

// In micro sec
#define WAIT_TIME_WHEN_IDLE 100000

// Used when refill from data_stack in comper
size_t MINI_BATCH_NUM = 40;

// Global big task queue
void *global_Qbig;
// Global big task queue mtx
mutex Qbig_mtx;

// A queue to manage big file names for spill/refill operations
conque<string> global_Lbig;
// Number of files in global_Lbig
atomic<int> global_Lbig_num;

size_t Qbig_capacity = 16;
// How many big tasks per file to spill
int BT_TASKS_PER_FILE = 4;
// A threshold to refill Qbig
int BT_THRESHOLD_FOR_REFILL = 8;
// Global regular task queue
void *global_Qreg;
// Global regular task queue mtx
mutex Qreg_mtx;

size_t Qreg_capacity = 512;
// How many reg tasks to spill.
int RT_TASKS_PER_FILE = 32;
// Threshold to refill Qreg.
int RT_THRESHOLD_FOR_REFILL = 128;

// A queue to manage reg file names for spill/refill operations
conque<string> global_Lreg;
// Number of files in global_Lreg
atomic<int> global_Lreg_num;
string TASK_DISK_BUFFER_DIR;

// Number of compers. Compers means threads
size_t num_compers = 32;
int BIGTASK_THRESHOLD = 200;

mutex data_stack_mtx;
void *global_data_stack;

// Number of tasks assigned to each comper
size_t tasks_per_fetch_g = 1;

condition_variable cv_go;
mutex mtx_go;
// Protected by mtx_go above
bool ready_go = true;

bool enable_log = false;
void log(const string &text)
{
	if (enable_log)
		cout << "Thread id: " << this_thread::get_id() << " " << text << endl;
}

bool enable_log_time = false;

typedef std::chrono::_V2::steady_clock::time_point timepoint;
typedef std::chrono::milliseconds ms;
typedef std::chrono::system_clock clk;

// Forward declaration used by output-summary initialization.
void recursive_mkdir(const char *dir);

// Runtime-configurable directory for per-thread output files.
string THREAD_OUTPUT_DIR = ".";

// Cleanup hook registration state for abnormal early exits via exit().
atomic<bool> buffer_cleanup_hook_registered(false);
atomic<bool> buffer_cleanup_started(false);

// Single summary output file path, resolved from THREAD_OUTPUT_DIR.
string THREAD_SUMMARY_FILE_PATH;

// Thread-output statistics keyed by thread index.
mutex output_stats_mtx;
vector<int> output_max_quasi_clique_size;
vector<double> output_first_time_of_max_sec;
vector<timepoint> output_start_time;
vector<int> output_thread_lb;
vector<double> output_first_time_of_lb_sec;
thread_local int output_thread_id = -1;
bool output_summary_open_error_logged = false;

inline bool ends_with(const string &value, const string &suffix)
{
	if (value.size() < suffix.size())
		return false;
	return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

inline string resolve_summary_output_path(const string &output_path)
{
	string resolved = output_path;
	if (resolved.empty())
		resolved = "thread_summary.txt";

	struct stat st;
	if (stat(resolved.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
	{
		if (!resolved.empty() && resolved.back() != '/')
			resolved += "/";
		resolved += "thread_summary.txt";
		return resolved;
	}

	if (!resolved.empty() && resolved.back() == '/')
	{
		resolved += "thread_summary.txt";
		return resolved;
	}

	if (ends_with(resolved, "thread_summary.txt") || ends_with(resolved, "_thread_summary.txt"))
		return resolved;

	// Treat other values as a base file path and append an explicit suffix,
	// so it never overwrites runsolver/main log output files.
	return resolved + "_thread_summary.txt";
}

inline void write_summary_snapshot_locked()
{
	if (THREAD_SUMMARY_FILE_PATH.empty())
		return;

	ofstream out(THREAD_SUMMARY_FILE_PATH, ios::out | ios::trunc);
	if (!out.is_open())
	{
		if (!output_summary_open_error_logged)
		{
			cerr << "[WARN] cannot open summary output file: " << THREAD_SUMMARY_FILE_PATH << endl;
			output_summary_open_error_logged = true;
		}
		return;
	}
	output_summary_open_error_logged = false;

	for (size_t i = 0; i < output_max_quasi_clique_size.size(); i++)
	{
		out << "thread=" << i
			<< " max_quasi_clique_size=" << output_max_quasi_clique_size[i]
			<< " first_time_sec=" << fixed << setprecision(6) << output_first_time_of_max_sec[i];
		//if (output_max_quasi_clique_size[i] == 0 && i < output_thread_lb.size() && i < output_first_time_of_lb_sec.size())
		{
			out << " lb=" << output_thread_lb[i]
				<< " lb_time_sec=" << fixed << setprecision(6) << output_first_time_of_lb_sec[i];
		}
		out << "\n";
	}
	out.flush();
}

inline void init_output_summary(const string &output_path, int thread_count)
{
	lock_guard<mutex> lk(output_stats_mtx);

	THREAD_SUMMARY_FILE_PATH = resolve_summary_output_path(output_path);

	size_t slash = THREAD_SUMMARY_FILE_PATH.find_last_of('/');
	if (slash != string::npos)
	{
		string parent = THREAD_SUMMARY_FILE_PATH.substr(0, slash);
		if (!parent.empty())
			recursive_mkdir(parent.c_str());
	}

	output_max_quasi_clique_size.assign(thread_count, 0);
	output_first_time_of_max_sec.assign(thread_count, -1.0);
	output_start_time.assign(thread_count, std::chrono::steady_clock::now());
	output_thread_lb.assign(thread_count, 0);
	output_first_time_of_lb_sec.assign(thread_count, -1.0);
	write_summary_snapshot_locked();
}

inline void set_output_thread_id(int thread_id)
{
	output_thread_id = thread_id;
	lock_guard<mutex> lk(output_stats_mtx);
	if (thread_id >= 0 && (size_t)thread_id < output_start_time.size())
		output_start_time[thread_id] = std::chrono::steady_clock::now();
}

inline void record_quasi_clique(int clique_size)
{
	if (clique_size <= 0 || output_thread_id < 0)
		return;

	lock_guard<mutex> lk(output_stats_mtx);
	if ((size_t)output_thread_id >= output_max_quasi_clique_size.size())
		return;

	double elapsed_sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - output_start_time[output_thread_id]).count();
	int &cur_max = output_max_quasi_clique_size[output_thread_id];
	double &first_time = output_first_time_of_max_sec[output_thread_id];

	if (clique_size > cur_max)
	{
		cur_max = clique_size;
		first_time = elapsed_sec;
		write_summary_snapshot_locked();
	}
}

inline void record_thread_lb(int lb)
{
	if (lb <= 0 || output_thread_id < 0)
		return;

	lock_guard<mutex> lk(output_stats_mtx);
	if ((size_t)output_thread_id >= output_thread_lb.size())
		return;
	if ((size_t)output_thread_id >= output_first_time_of_lb_sec.size())
		return;

	double elapsed_sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - output_start_time[output_thread_id]).count();

	int &cur_lb = output_thread_lb[output_thread_id];
	double &first_time = output_first_time_of_lb_sec[output_thread_id];
	if (lb > cur_lb)
	{
		cur_lb = lb;
		first_time = elapsed_sec;
		if (output_max_quasi_clique_size[output_thread_id] == 0)
			write_summary_snapshot_locked();
	}
}

mutex cout_mtx;
float log_time(const string msg, const timepoint start_time, const float latest_elapsed_time)
{
	float elapsed_time = 0;
	if (enable_log_time)
	{
		cout_mtx.lock();
		auto end_time = std::chrono::_V2::steady_clock::now();
		elapsed_time = (float)std::chrono::duration_cast<ms>(end_time - start_time).count() / 1000;
		time_t date_time_now = clk::to_time_t(clk::now());
		char *ctime_no_newline = strtok(ctime(&date_time_now), "\n"); // strtok() to replace \n with \0

		cout << "Thread id: " << this_thread::get_id() << ", " << ctime_no_newline << ", "
			 << msg << ", elapsed_time:" << elapsed_time << " seconds"
			 << ", step time," << elapsed_time - latest_elapsed_time << endl;
		cout_mtx.unlock();
	}
	return elapsed_time;
}

// Disk operations
void make_directory(const char *name)
{
#ifdef __linux__ // check if linux
	mkdir(name, S_IRWXU);
#else
	// Works on Windows when inclue direct.h, but direct.h doesn't work on linux as it was "provided by Microsoft Windows".
	_mkdir(name); 
#endif
}

// Reference: http://nion.modprobe.de/blog/archives/357-Recursive-directory-creation.html
void recursive_mkdir(const char *dir)
{
	char tmp[256];
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", dir);
	len = strlen(tmp);
	if (tmp[len - 1] == '/')
		tmp[len - 1] = '\0';
	for (p = tmp + 1; *p; p++)
		if (*p == '/')
		{
			*p = 0;
			// Supports both Linux and Windows
			make_directory(tmp);
			*p = '/';
		}
	make_directory(tmp);
}

	inline bool remove_file_if_exists(const string &path)
	{
		if (path.empty())
			return false;
		if (remove(path.c_str()) == 0)
			return true;
		if (errno == ENOENT)
			return false;
		return false;
	}

	inline void cleanup_directory_files(const string &dir_path, bool remove_dir)
	{
		if (dir_path.empty())
			return;

		DIR *dir = opendir(dir_path.c_str());
		if (dir == NULL)
			return;

		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL)
		{
			const char *name = entry->d_name;
			if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
				continue;

			string full = dir_path;
			if (!full.empty() && full.back() != '/')
				full += "/";
			full += name;

			struct stat st;
			if (stat(full.c_str(), &st) != 0)
				continue;

			if (S_ISREG(st.st_mode))
				remove_file_if_exists(full);
			else if (S_ISDIR(st.st_mode))
				cleanup_directory_files(full, true);
		}
		closedir(dir);

		if (remove_dir)
			rmdir(dir_path.c_str());
	}

	inline bool parse_run_dir_pid(const string &name, pid_t &pid_out)
	{
		pid_out = -1;
		if (name.size() <= 5 || name.compare(0, 4, "run_") != 0)
			return false;

		size_t pos = 4;
		if (pos >= name.size() || name[pos] < '0' || name[pos] > '9')
			return false;

		long long pid_value = 0;
		while (pos < name.size() && name[pos] >= '0' && name[pos] <= '9')
		{
			pid_value = pid_value * 10 + (name[pos] - '0');
			pos++;
		}

		if (pos >= name.size() || name[pos] != '_')
			return false;

		if (pid_value <= 0)
			return false;

		pid_out = (pid_t)pid_value;
		return true;
	}

	inline bool is_process_alive(pid_t pid)
	{
		if (pid <= 0)
			return false;
		if (kill(pid, 0) == 0)
			return true;
		return errno == EPERM;
	}

	inline bool read_process_starttime(pid_t pid, unsigned long long &start_time_out)
	{
		start_time_out = 0;
		if (pid <= 0)
			return false;

		char proc_path[64];
		snprintf(proc_path, sizeof(proc_path), "/proc/%d/stat", (int)pid);
		FILE *fp = fopen(proc_path, "rt");
		if (fp == NULL)
			return false;

		char line[8192];
		char *ret = fgets(line, sizeof(line), fp);
		fclose(fp);
		if (ret == NULL)
			return false;

		char *rparen = strrchr(line, ')');
		if (rparen == NULL)
			return false;

		char *p = rparen + 1;
		int field_no = 3;
		while (*p != '\0' && field_no <= 22)
		{
			while (*p == ' ' || *p == '\t')
				p++;
			if (*p == '\0' || *p == '\n')
				break;

			char *end = p;
			while (*end != '\0' && *end != ' ' && *end != '\t' && *end != '\n')
				end++;

			if (field_no == 22)
			{
				char saved = *end;
				*end = '\0';
				start_time_out = strtoull(p, NULL, 10);
				*end = saved;
				return start_time_out > 0;
			}

			field_no++;
			p = end;
		}

		return false;
	}

	inline void cleanup_stale_buffer_runs(const string &buffer_root)
	{
		DIR *dir = opendir(buffer_root.c_str());
		if (dir == NULL)
			return;

		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL)
		{
			const char *name = entry->d_name;
			if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
				continue;

			string full = buffer_root;
			if (!full.empty() && full.back() != '/')
				full += "/";
			full += name;

			struct stat st;
			if (stat(full.c_str(), &st) != 0)
				continue;

			if (S_ISREG(st.st_mode))
			{
				// Backward compatibility: old spill format wrote files directly under buffered_tasks.
				remove_file_if_exists(full);
				continue;
			}

			if (!S_ISDIR(st.st_mode))
				continue;

			string dir_name(name);
			pid_t run_pid = -1;
			if (!parse_run_dir_pid(dir_name, run_pid) || !is_process_alive(run_pid))
				cleanup_directory_files(full, true);
		}

		closedir(dir);
	}

	inline void cleanup_buffer_dir_on_exit()
	{
		cleanup_directory_files(TASK_DISK_BUFFER_DIR, true);
	}

	inline void cleanup_buffer_dir_on_exit_once()
	{
		bool expected = false;
		if (!buffer_cleanup_started.compare_exchange_strong(expected, true))
			return;
		cleanup_buffer_dir_on_exit();
	}

	inline void emergency_cleanup_and_exit(int signal_no)
	{
			(void)signal_no;
			// Async-signal context: only request cooperative shutdown.
			global_stop_requested = true;
	}

	inline void register_buffer_cleanup_hook_once()
	{
		bool expected = false;
		if (buffer_cleanup_hook_registered.compare_exchange_strong(expected, true))
		{
			atexit(cleanup_buffer_dir_on_exit_once);
		}
	}

	inline string get_executable_dir()
	{
	#ifdef __linux__
		char path_buf[4096];
		ssize_t len = readlink("/proc/self/exe", path_buf, sizeof(path_buf) - 1);
		if (len > 0)
		{
			path_buf[len] = '\0';
			string exe_path(path_buf);
			size_t slash = exe_path.find_last_of('/');
			if (slash != string::npos)
				return exe_path.substr(0, slash);
		}
	#endif

		char cwd_buf[4096];
		if (getcwd(cwd_buf, sizeof(cwd_buf)) != NULL)
			return string(cwd_buf);
		return ".";
	}

	inline string get_buffer_root_dir()
	{
		string exe_dir = get_executable_dir();
		if (exe_dir.empty() || exe_dir == ".")
			return "buffered_tasks";
		return exe_dir + "/buffered_tasks";
	}

	inline void start_detached_buffer_cleaner(const string &run_dir)
	{
	#ifdef __linux__
		if (run_dir.empty())
			return;

		pid_t parent_pid = getpid();
		unsigned long long parent_starttime = 0;
		if (!read_process_starttime(parent_pid, parent_starttime))
			return;

		pid_t child_pid = fork();
		if (child_pid < 0)
			return;
		if (child_pid > 0)
		{
			int status = 0;
			waitpid(child_pid, &status, 0);
			return;
		}

		pid_t grandchild_pid = fork();
		if (grandchild_pid < 0)
			_exit(0);
		if (grandchild_pid > 0)
			_exit(0);

		setsid();
		while (true)
		{
			unsigned long long cur_starttime = 0;
			if (!read_process_starttime(parent_pid, cur_starttime) || cur_starttime != parent_starttime)
				break;
			usleep(100000);
		}

		cleanup_directory_files(run_dir, true);
		_exit(0);
	#else
		(void)run_dir;
	#endif
	}

#endif