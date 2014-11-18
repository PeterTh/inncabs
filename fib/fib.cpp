#include "../include/inncabs.h"

typedef long long ll;

ll fib(int n, const std::launch l) {
	if(n < 2) return n;

	auto x = std::async(l, fib, n - 1, l);
	auto y = std::async(l, fib, n - 2, l);

	return x.get() + y.get();
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
		[n](const std::launch l) { return fib(n, l); },
		[n](ll result) { return result == fib_verify_value(n); },
		ss.str() 
		);
}
