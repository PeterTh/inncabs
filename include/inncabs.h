#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <future>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <vector>
#include <tuple>
#include <string>
#include <chrono>
#include <numeric>
#include <iomanip>
#include <cstdlib>
#include <cstdio>
#include <cmath>

namespace {
	bool readEnvBool(const char* envName) {
		const char* val = getenv(envName);
		if(!val) return false;
		const static std::vector<std::string> trueSynonyms {"true", "True", "TRUE", "enabled", "Enabled", "ENABLED", "on", "On", "ON", "1" };
		return std::any_of(trueSynonyms.cbegin(), trueSynonyms.cend(), [val](const std::string& str) { return str == val; });
	}

	template<typename T>
	double mean(std::vector<T> vec) {
		if(vec.size() == 1) return static_cast<double>(vec[0]);
		double sum = 0.0;
		for(const auto& e : vec) sum += e;
		return sum / vec.size();
	}

	template<typename T>
	T median(std::vector<T> vec) {
		if(vec.size() == 1) return vec[0];
		std::sort(vec.begin(), vec.end());
		if(vec.size() % 2 == 0) {
			return (vec[vec.size() / 2] + vec[vec.size() / 2 + 1]) / 2;
		}
		else {
			return vec[vec.size() / 2];
		}
	}

	template<typename T>
	double stddev(std::vector<T> vec) {
		if(vec.size() == 1) return 0.0;
		double m = mean(vec);
		double dsum = 0.0;
		for(const auto& e : vec) dsum += pow((e-m), 2);
		return sqrt(dsum / vec.size());
	}
}

namespace inncabs {

//#define COUNTING_DEFER

#if defined(INNCABS_USE_HPX)
#ifdef COUNTING_DEFER
	namespace counting_defer
	{
		using hpx::async;
	}
#else
	using hpx::async;
#endif
	using hpx::launch;
	using hpx::future;
	using hpx::thread;
	namespace this_thread { using hpx::this_thread::sleep_for; }
	using hpx::lcos::local::mutex;
#else
#ifdef COUNTING_DEFER
	namespace counting_defer
	{
		using std::async;
	}
#else
	using std::async;
#endif
	using std::launch;
	using std::future;
	using std::thread;
	namespace this_thread { using std::this_thread::sleep_for; }
	using std::mutex;
#endif

#ifdef COUNTING_DEFER
	std::atomic<unsigned> g_par_count;

	template < class Function, class... Args >
	auto async(inncabs::launch policy, Function&& f, Args&&... args)
		-> typename std::enable_if< std::is_void<decltype(f(args...))>::value, decltype(inncabs::counting_defer::async(policy, f, args...))>::type {
		if(policy != (inncabs::launch::deferred | inncabs::launch::async)) return inncabs::counting_defer::async(policy, f, args...);
		if(g_par_count.fetch_add(1) < inncabs::thread::hardware_concurrency()) {
			return inncabs::counting_defer::async(inncabs::launch::async, [&]() {
				f(args...);
				g_par_count.fetch_sub(1);
			});
		}
		else {
			g_par_count.fetch_sub(1);
		}
		return inncabs::counting_defer::async(inncabs::launch::deferred, f, args...);
	}

	template < class Function, class... Args >
	auto async(inncabs::launch policy, Function&& f, Args&&... args)
		-> typename std::enable_if< !std::is_void<decltype(f(args...))>::value, decltype(inncabs::counting_defer::async(policy, f, args...))>::type {
		if(policy != (inncabs::launch::deferred | inncabs::launch::async)) return inncabs::counting_defer::async(policy, f, args...);
		if(g_par_count.fetch_add(1) < inncabs::thread::hardware_concurrency()) {
			return inncabs::counting_defer::async(inncabs::launch::async, [&]() {
				auto ret = f(args...);
				g_par_count.fetch_sub(1);
				return ret;
			});
		}
		else {
			g_par_count.fetch_sub(1);
		}
		return inncabs::counting_defer::async(inncabs::launch::deferred, f, args...);
	}
#endif

	const static char* ENV_VAR_CSV = "INNCABS_CSV_OUTPUT";
	const static char* ENV_VAR_MIN = "INNCABS_MIN_OUTPUT";
	const static char* ENV_VAR_REPEATS = "INNCABS_REPEATS";
	const static char* ENV_VAR_LAUNCH = "INNCABS_LAUNCH_TYPES";
	const static char* ENV_VAR_TIMEOUT = "INNCABS_TIMEOUT";

	using BenchResult = std::tuple<bool, long long>;
	using LaunchConfiguration = std::tuple<inncabs::launch, std::string>;

	template<typename Executor, typename Checker>
	BenchResult benchmark(Executor x, Checker c, const inncabs::launch l, const std::function<void()>& initializer) {
		initializer();
		std::chrono::high_resolution_clock highc;
		auto start = highc.now();
		auto r =  x(l);
		auto end = highc.now();
		return BenchResult(c(r), std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
	}

	template<typename Executor, typename Checker>
	void run_all(Executor x, Checker c, const std::string& bench, const std::function<void()>& initializer = [](){}) {
		// read environment variables
		bool csvoutput = readEnvBool(ENV_VAR_CSV);
		bool minoutput = readEnvBool(ENV_VAR_MIN);
		std::string configSelection = "deferred,async,optional";
		if(getenv(ENV_VAR_LAUNCH)) configSelection = getenv(ENV_VAR_LAUNCH);
		unsigned repeats = 1;
		if(getenv(ENV_VAR_REPEATS)) repeats = std::atol(getenv(ENV_VAR_REPEATS));
		std::chrono::milliseconds timeout{ 0 };
		if(getenv(ENV_VAR_TIMEOUT)) timeout = std::chrono::milliseconds(std::atol(getenv(ENV_VAR_TIMEOUT)));

		// start timeout if requested
		bool done = false;
		if(timeout != std::chrono::milliseconds(0)) {
			auto t = inncabs::thread([&] {
				inncabs::this_thread::sleep_for(timeout);
				if(!done) exit(-1);
			});
			t.detach();
		}

		// write header
		if(csvoutput) {
			std::cout << std::setw(16) << bench << ", success" << ", time (ms)" << ", stddev" << std::endl;
		}
		else if(!minoutput) {
			std::cout << "Benchmarking " << bench << std::endl;
		}

		// perform benchmarks
		std::vector<LaunchConfiguration> configurations {
			LaunchConfiguration { inncabs::launch::deferred, "deferred" },
			LaunchConfiguration { inncabs::launch::deferred | inncabs::launch::async, "optional" },
			LaunchConfiguration { inncabs::launch::async, "async" } };

		for(const auto& config : configurations) {
			if(configSelection.find(std::get<1>(config)) != std::string::npos) {
				BenchResult res;
				std::vector<long long> times;
				for(unsigned i = 0; i < repeats; ++i) {
					res = benchmark(x, c, std::get<0>(config), initializer);
					times.push_back(std::get<1>(res));
				}
				long long mid_time = median(times);
				double std_dev = stddev(times);
				if(minoutput) {
					std::cout << mid_time << "," << std_dev << std::endl;
				} else if(csvoutput) {
					std::cout << std::setw(16) << std::get<1>(config)
						<< std::setw(2) << ", " << std::setw(14) << std::get<0>(res)
						<< std::setw(2) << ", " << std::setw(14) << mid_time
						<< std::setw(2) << ", " << std::setw(14) << std_dev << std::endl;
				}
				else {
					std::cout << "launch: " << std::get<1>(config) << std::endl
						<< "success: " << (std::get<0>(res) ? "SUCCESSFUL" : "FAILED") << std::endl
						<< "time: " << mid_time << " ms" << std::endl
						<< "stddev: " << std_dev << std::endl;
				}
			}
		}

		done = true;
	}

	void message(const std::string& msg) {
		#ifdef INNCABS_MSG
		std::cout << msg;
		#endif
	}

	void debug(const std::string& msg) {
		#ifdef INNCABS_DEBUG
		std::cout << msg;
		#endif
	}

	void error(const std::string& msg) {
		std::cerr << msg;
		exit(-1);
	}
}
