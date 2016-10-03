#if defined(INNCABS_USE_HPX)
#include <hpx/hpx_main.hpp>
#include <hpx/hpx.hpp>
#endif

#include "../include/inncabs.h"

#include <random>

class Philosopher {
    std::mt19937_64 eng_{std::random_device{}()};

    inncabs::mutex& left_fork_;
    inncabs::mutex& right_fork_;
    std::chrono::milliseconds eat_time_{0};

public:
    static std::chrono::milliseconds full;
    Philosopher(inncabs::mutex& left, inncabs::mutex& right);
    void dine();

private:
    void eat();
    bool flip_coin();
    std::chrono::milliseconds get_eat_duration();
};

std::chrono::milliseconds Philosopher::full;

Philosopher::Philosopher(inncabs::mutex& left, inncabs::mutex& right)
    : left_fork_(left)
    , right_fork_(right)
{}

void Philosopher::dine() {
    while(eat_time_ < full) eat();
}

void Philosopher::eat() {
    using Lock = std::unique_lock<inncabs::mutex>;
    Lock first;
    Lock second;
    if (flip_coin())
    {
        first = Lock(left_fork_, std::defer_lock);
        second = Lock(right_fork_, std::defer_lock);
    }
    else
    {
        first = Lock(right_fork_, std::defer_lock);
        second = Lock(left_fork_, std::defer_lock);
    }
    auto d = get_eat_duration();
    std::lock(first, second);
    auto end = std::chrono::steady_clock::now() + d;
    while(std::chrono::steady_clock::now() < end);
    eat_time_ += d;
}

bool Philosopher::flip_coin() {
    std::bernoulli_distribution d;
    return d(eng_);
}

std::chrono::milliseconds Philosopher::get_eat_duration() {
    std::uniform_int_distribution<> ms(1, 10);
    return (std::min)(std::chrono::milliseconds(ms(eng_)), full - eat_time_);
}

int main(int argc, char** argv) {
	unsigned n = 16;
	if(argc>1) n = std::atoi(argv[1]);
	Philosopher::full = std::chrono::milliseconds(50);
	if(argc>2) Philosopher::full = std::chrono::milliseconds(std::atoi(argv[2]));

	std::vector<inncabs::mutex> table(n);
	std::vector<Philosopher> diners;

	std::stringstream ss;
	ss << "Dining Philosophers (N = " << n << ")";

	inncabs::run_all(
		[&](const inncabs::launch l) {
			std::vector<inncabs::future<void>> futures;
			for(auto& d : diners) {
				futures.push_back(inncabs::async(l, [&] { d.dine(); }));
			}
			for(auto& f : futures) {
				f.wait();
			}
			return true;
		},
		[&](bool result) {
			return result;
		},
		ss.str(),
		[&] {
			diners.clear();
			for(unsigned i = 0; i < table.size(); ++i) {
				int k = i < table.size() - 1 ? i + 1 : 0;
				diners.push_back(Philosopher(table[i], table[k]));
			}
		}
	);
    return 0;
}
