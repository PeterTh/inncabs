#pragma once

#include "parec/core.h"

/*
 Adapted from the Insieme compiler "qap" test case.
 Copyright 2012-2014 University of Innsbruck
 */

// A quadratic matrix, dynamically sized
typedef struct _qmatrix {
	int size;
	int data[];
} qmatrix;

qmatrix* qm_create(int size) {
	qmatrix* res = (qmatrix*)malloc(sizeof(qmatrix) + size * size * sizeof(int));
	res->size = size;
	return res;
}

void qm_del(qmatrix* matrix) {
	free(matrix);
}

// a struct describing a QAP instance
typedef struct _problem {
	int size;		// the size of the problem
	qmatrix* A;		// the weight matrix (size x size)
	qmatrix* B;		// the distance matrix (size x size)
	int optimum;		// the value of the optimal solution
} problem;

void qap_del(problem* problem) {
	qm_del(problem->A);
	qm_del(problem->B);
	free(problem);
}

#define getM(M,I,J) M->data[I*M->size+J]
#define getA(P,I,J) getM(P->A,I,J)
#define getB(P,I,J) getM(P->B,I,J)


// a struct representing a (partial) solution to the problem
typedef struct _solution {
	struct _solution* head;		// the solution is forming a linked list
	int pos;			// the location the current facility is assigned to
} solution;

solution* empty() { return 0; }

void print(solution* solution) {
	if(!solution) return;
	print(solution->head);
	printf("-%d", solution->pos);
}

struct params {
	problem* problem_;
	solution* partial;
	int plant;
	int used_mask;
	int cur_cost;
	std::atomic<int>& best_known;
};


template<typename T>
int solve_rec_step(const params& p, const T& rec_call) {
	problem* problem = p.problem_;
	solution* partial = p.partial;
	int plant = p.plant;
	int used_mask = p.used_mask;
	int cur_cost = p.cur_cost;
	std::atomic<int>& best_known = p.best_known;

	if(cur_cost >= best_known) {
		return best_known;
	}

	std::vector<decltype(rec_call(p))> futures;

	solution tmp[problem->size];

	// fix current position
	for(int i=0; i<problem->size; i++) {
		// check whether current spot is a free spot
		if(!(1<<i & used_mask)) {
			// extend solution
			tmp[i] = {partial, i};

			// compute additional cost of current assignment
			int new_cost = 0;

			int cur_plant = plant;
			solution* cur = &tmp[i];
			while(cur) {
				int other_pos = cur->pos;

				// add costs between current pair of plants
				new_cost += getA(problem, plant, cur_plant) * getB(problem, i, other_pos);
				new_cost += getA(problem, cur_plant, plant) * getB(problem, other_pos, i);

				// go to next plant
				cur = cur->head;
				cur_plant--;
			}

			// compute recursive rest
			futures.push_back(rec_call({problem, &tmp[i], plant+1, used_mask | (1<<i), cur_cost + new_cost, best_known}));
		}
	}

	for(auto& f : futures) {
		auto cur_best = f.get();
		// update best known solution
		if(cur_best < best_known) {
			int best;
			//   |--- read best ---|         |--- check ---|    |------- update if cur_best is better ------------|
			do { best = best_known; } while (cur_best < best && best_known.compare_exchange_strong(best, cur_best));
		}
	}

	return best_known;
}

int solve(const std::launch l, problem* problem) {
	solution* map = empty();
	std::atomic<int> best{ 1 << 30 };

	auto rec_solver = parec::prec(
		[](const params& p){
			return p.plant >= p.problem_->size;
		},
		[](const params& p) {
			return p.cur_cost;
		},
		[](const params& p, const auto& rec_call) {
			return solve_rec_step(p, rec_call);
		}
	);

	return rec_solver({problem, map, 0, 0, 0, best}).get();
}

problem* qap_load(const char* file) {
	FILE* fp = fopen(file, "r");
	std::stringstream ss;
	ss << "Loading Problem File " << file << " ...\n";

	// get problem size
	int problemSize;
	fscanf(fp, "%d", &problemSize);
	ss << "  - problem size: << " << problemSize << std::endl;

	// create problem instance
	problem* res = (problem*)malloc(sizeof(problem));
	res->size = problemSize;
	res->A = qm_create(problemSize);
	res->B = qm_create(problemSize);

	// load matrix A
	for(int i=0; i<problemSize; i++) {
		for(int j=0; j<problemSize; j++) {
			fscanf(fp, "%d", &getA(res,i,j));
		}
	}

	// load matrix B
	for(int i=0; i<problemSize; i++) {
		for(int j=0; j<problemSize; j++) {
			fscanf(fp, "%d", &getB(res,i,j));
		}
	}

	// load optimum
	fscanf(fp, "%d", &(res->optimum));
	ss << "  - optimum: << " << res->optimum << std::endl;
	inncabs::message(ss.str());

	fclose(fp);
	return res;
}
