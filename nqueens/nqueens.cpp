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

ll solutions(const int n, const std::launch l, const int col = 0, history h = history()) {
	if(col == n) {
		return 1;
	} else {
		std::vector<std::future<ll>> futures;
		for(int row=0; row<n; ++row) {
			history x = h;
			x.push_back(row);
			if(valid(n, col+1, x)) futures.push_back(std::async(l, solutions, n, l, col+1, x));
		}
		return accumulate(futures.begin(), futures.end(), 0ll,
			[](ll sum, std::future<ll>& b) { return sum + b.get(); });
	}
}

static const ll CHECK[] = { 1,0,0,2,10,4,40,92,352,724,2680,14200,73712,365596,2279184,14772512,95815104,666090624,4968057848,39029188884,314666222712 };

int main(int argc, char** argv) {
	int n = 8;
	if(argc > 1) n = atoi(argv[1]);

	std::stringstream ss;
	ss << "N-Queens N=" << n;

	inncabs::run_all(
		[n](const std::launch l) { return solutions(n, l); },
		[n](ll result) { return result == CHECK[n-1]; },
		ss.str() 
		);
}
