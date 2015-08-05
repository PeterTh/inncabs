#if defined(INNCABS_USE_HPX)
#include <hpx/hpx_main.hpp>
#include <hpx/hpx.hpp>
#endif

#include "../include/inncabs_wrapped.h"

typedef long long ll;

ll fib(int n, const inncabs::launch l) {
	if(n < 2) return n;

	auto x = inncabs::async(l, fib, n - 1, l);
	auto y = inncabs::async(l, fib, n - 2, l);

	return x.get() + y.get();
}

std::tuple<ll, long long> fib_wrapped(int n, const inncabs::launch l) {
    // start the time counter
    std::chrono::high_resolution_clock highc;
    auto start = highc.now();
    // exec
    auto result = fib(n, l);
    auto end = highc.now();
    return std::tuple<ll, long long>(result, std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());
}

static const ll FIB_RESULTS_PRE = 41;
static const ll fib_results[FIB_RESULTS_PRE] = {0,1,1,2,3,5,8,13,21,34,55,89,144,233,377,610,987,1597,2584,4181,6765,10946,17711,28657,46368,75025,121393,196418,317811,514229,832040,1346269,2178309,3524578,5702887,9227465,14930352,24157817,39088169,63245986,102334155};

ll fib_verify_value(int n) {
	if(n < FIB_RESULTS_PRE) return fib_results[n];
	return fib_verify_value(n-1) + fib_verify_value(n-2);
}

int main(int argc, char** argv) {
	int n = 12;
	if(argc > 1) n = atoi(argv[1]);

	std::stringstream ss;
	ss << "Fibonacci N=" << n;

	inncabs::run_all(
		[n](const inncabs::launch l) { return fib_wrapped(n, l); },
		[n](ll result) { return result == fib_verify_value(n); },
		ss.str()
		);
    return 0;
}
