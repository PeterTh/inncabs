#if defined(INNCABS_USE_HPX)
#include <hpx/hpx_main.hpp>
#include <hpx/hpx.hpp>
#endif

#include "../include/inncabs.h"

typedef long long ll;
typedef std::vector<int> history;

bool valid(const int n, const int col, const history& h) {
	if(col==0) return true;
	int row = h[col-1];
	for(int prev_col = 0; prev_col<col-1; ++prev_col) {
		int prev_row = h[prev_col];
		if(row == prev_row) return false;
		if((col-1)-prev_col == abs(row-prev_row)) return false;
	}
	return true;
}

int threshold = 8;

ll seq_solutions(const int n, const inncabs::launch l, const int col = 0, history h = history())
{
    if(col == n) {
        return 1;
    } else {
        std::vector<ll> results;
        for(int row=0; row<n; ++row) {
            history x = h;
            x.push_back(row);
            if(valid(n, col+1, x))
                results.push_back(seq_solutions(n, l, col+1, std::move(x)));
        }
        return accumulate(results.begin(), results.end(), 0ll);
    }
}

#if defined(INNCABS_USE_HPX_FUTURIZED)
inncabs::future<ll> solutions(const int n, const inncabs::launch l, const int col = 0, history h = history()) {
    if(col == n) {
        return hpx::make_ready_future(1LL);
    } else if (col > n - threshold) {
        return hpx::make_ready_future(seq_solutions(n, l, col, std::move(h)));
    } else {
        std::vector<inncabs::future<ll>> futures;
        for(int row=0; row<n; ++row) {
            history x = h;
            x.push_back(row);
            if(valid(n, col+1, x))
                futures.push_back(inncabs::async(l, solutions, n, l, col+1, std::move(x)));
        }
        return hpx::lcos::local::dataflow(
            [](std::vector<inncabs::future<ll>> && futures)
            {
                return accumulate(futures.begin(), futures.end(), 0ll,
                    [](ll sum, inncabs::future<ll>& b)
                    {
                        return sum + b.get();
                    });
            },
            futures);
    }
}
#else
ll solutions(const int n, const inncabs::launch l, const int col = 0, history h = history()) {
	if(col == n) {
		return 1;
    } else if (col > n - threshold) {
        return seq_solutions(n, l, col, std::move(h));
	} else {
		std::vector<inncabs::future<ll>> futures;
		for(int row=0; row<n; ++row) {
			history x = h;
			x.push_back(row);
			if(valid(n, col+1, x)) futures.push_back(inncabs::async(l, solutions, n, l, col+1, std::move(x)));
		}
		return accumulate(futures.begin(), futures.end(), 0ll,
			[](ll sum, inncabs::future<ll>& b) { return sum + b.get(); });
	}
}
#endif

static const ll CHECK[] = { 1,0,0,2,10,4,40,92,352,724,2680,14200,73712,365596,2279184,14772512,95815104,666090624,4968057848,39029188884,314666222712 };

int main(int argc, char** argv) {
	int n = 8;
	if(argc > 1) n = atoi(argv[1]);
	if(argc > 2) threshold = atoi(argv[2]);

	std::stringstream ss;
	ss << "N-Queens N=" << n;

	inncabs::run_all(
		[n](const inncabs::launch l)
        {
#if defined(INNCABS_USE_HPX_FUTURIZED)
            return solutions(n, l).get();
#else
            return solutions(n, l);
#endif
        },
		[n](ll result) { return result == CHECK[n-1]; },
		ss.str()
		);
    return 0;
}
