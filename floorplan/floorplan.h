#pragma once

#include "allscale/api/core/prec.h"

#include <vector>
#include <type_traits>
#include <assert.h>
#include <string.h>

#define ROWS 64
#define COLS 64
#define DMAX 64
#define max(a, b) ((a > b) ? a : b)
#define min(a, b) ((a < b) ? a : b)

int solution = -1;

typedef int  coor[2];
typedef char ibrd[ROWS][COLS];
typedef char (*pibrd)[COLS];

FILE * inputFile;

struct cell {
	int   n;
	coor *alt;
	int   top;
	int   bot;
	int   lhs;
	int   rhs;
	int   left;
	int   above;
	int   next;
};

struct cell * gcells;

int  MIN_AREA;
ibrd BEST_BOARD;
coor MIN_FOOTPRINT;

int N;

/* compute all possible locations for nw corner for cell */
static int starts(int id, int shape, coor *NWS, struct cell *cells) {
	int i, n, top, bot, lhs, rhs;
	int rows, cols, left, above;

	/* size of cell */
	rows  = cells[id].alt[shape][0];
	cols  = cells[id].alt[shape][1];

	/* the cells to the left and above */
	left  = cells[id].left;
	above = cells[id].above;

	/* if there is a vertical and horizontal dependence */
	if ((left >= 0) && (above >= 0)) {

		top = cells[above].bot + 1;
		lhs = cells[left].rhs + 1;
		bot = top + rows;
		rhs = lhs + cols;

		/* if footprint of cell touches the cells to the left and above */
		if ((top <= cells[left].bot) && (bot >= cells[left].top) &&
			(lhs <= cells[above].rhs) && (rhs >= cells[above].lhs))
		{ n = 1; NWS[0][0] = top; NWS[0][1] = lhs;  }
		else { n = 0; }

		/* if there is only a horizontal dependence */
	} else if (left >= 0) {

		/* highest initial row is top of cell to the left - rows */
		top = max(cells[left].top - rows + 1, 0);
		/* lowest initial row is bottom of cell to the left */
		bot = min(cells[left].bot, ROWS);
		n   = bot - top + 1;

		for (i = 0; i < n; i++) {
			NWS[i][0] = i + top;
			NWS[i][1] = cells[left].rhs + 1;
		}

	} else {

		/* leftmost initial col is lhs of cell above - cols */
		lhs = max(cells[above].lhs - cols + 1, 0);
		/* rightmost initial col is rhs of cell above */
		rhs = min(cells[above].rhs, COLS);
		n   = rhs - lhs + 1;

		for (i = 0; i < n; i++) {
			NWS[i][0] = cells[above].bot + 1;
			NWS[i][1] = i + lhs;
		}  }

	return (n);
}



/* lay the cell down on the board in the rectangular space defined
by the cells top, bottom, left, and right edges. If the cell can
not be layed down, return 0; else 1.
*/
static int lay_down(int id, ibrd board, struct cell *cells) {
	int  i, j, top, bot, lhs, rhs;

	top = cells[id].top;
	bot = cells[id].bot;
	lhs = cells[id].lhs;
	rhs = cells[id].rhs;

	for (i = top; i <= bot; i++) {
		for (j = lhs; j <= rhs; j++) {
			if (board[i][j] == 0) board[i][j] = (char)id;
			else                  return(0);
		} }

	return (1);
}


#define read_integer(file,var) \
	if(fscanf(file, "%d", &var) == EOF) {\
	inncabs::error(" Bogus input file\n");\
	}

static void read_inputs() {
	int i, j, n;

	read_integer(inputFile,n);
	N = n;

	gcells = (struct cell *) malloc((n + 1) * sizeof(struct cell));

	gcells[0].n     =  0;
	gcells[0].alt   =  0;
	gcells[0].top   =  0;
	gcells[0].bot   =  0;
	gcells[0].lhs   = -1;
	gcells[0].rhs   = -1;
	gcells[0].left  =  0;
	gcells[0].above =  0;
	gcells[0].next  =  0;

	for (i = 1; i < n + 1; i++) {

		read_integer(inputFile, gcells[i].n);
		gcells[i].alt = (coor *) malloc(gcells[i].n * sizeof(coor));

		for (j = 0; j < gcells[i].n; j++) {
			read_integer(inputFile, gcells[i].alt[j][0]);
			read_integer(inputFile, gcells[i].alt[j][1]);
		}

		read_integer(inputFile, gcells[i].left);
		read_integer(inputFile, gcells[i].above);
		read_integer(inputFile, gcells[i].next);
	}

	if (!feof(inputFile)) {
		read_integer(inputFile, solution);
	}
}


static void write_outputs() {
	std::stringstream ss;
	ss << "Minimum area = " << MIN_AREA << "\n\n";

	for(int i = 0; i < MIN_FOOTPRINT[0]; i++) {
		for(int j = 0; j < MIN_FOOTPRINT[1]; j++) {
			if(BEST_BOARD[i][j] == 0) {
				ss << " ";
			}
			else {
				ss << (char)(((int)BEST_BOARD[i][j]) - 1 + ((int)'A'));
			}
		}
		ss << "\n";
	}

	inncabs::message(ss.str());
}

struct params {
	int id;
	coor& FOOTPRINT;
	ibrd& BOARD;
	struct cell *CELLS;
};

using coor_dmax = coor[DMAX];

struct inner_params {
	std::atomic_int_least32_t& nnc;
	struct cell *CELLS;
	coor_dmax& NWS;
	ibrd& BOARD;
	coor& FOOTPRINT;
	int i;
	int j;
	int id;
};

std::mutex m;

template<typename O, typename I>
int add_cell(const params& p, const O& outer_step, const I& inner_step);

template<typename O, typename I>
int add_cell_inner(const inner_params& p, const O& outer_step, const I& inner_step) {
	std::atomic_int_least32_t& nnc = p.nnc;
	struct cell *CELLS = p.CELLS;
	coor_dmax& NWS = p.NWS;
	ibrd& BOARD = p.BOARD;
	coor& FOOTPRINT = p.FOOTPRINT;
	int i = p.i;
	int j = p.j;
	int id = p.id;

	ibrd board;
	coor footprint;
	struct cell* cells = (struct cell*)alloca((N + 1)*sizeof(struct cell));
	memcpy(cells,CELLS,sizeof(struct cell)*(N+1));
	/* extent of shape */
	cells[id].top = NWS[j][0];
	cells[id].bot = cells[id].top + cells[id].alt[i][0] - 1;
	cells[id].lhs = NWS[j][1];
	cells[id].rhs = cells[id].lhs + cells[id].alt[i][1] - 1;

	memcpy(board, BOARD, sizeof(ibrd));

	/* if the cell cannot be layed down, prune search */
	if(!lay_down(id, board, cells)) {
		std::stringstream ss;
		ss << "Chip " << id << ", shape " << i << " does not fit\n";
		inncabs::debug(ss.str());
	} else {
		/* calculate new footprint of board and area of footprint */
		footprint[0] = max(FOOTPRINT[0], cells[id].bot+1);
		footprint[1] = max(FOOTPRINT[1], cells[id].rhs+1);
		int area         = footprint[0] * footprint[1];

		/* if last cell */
		if (cells[id].next == 0) {

			/* if area is minimum, update global values */
			if (area < MIN_AREA) {
				m.lock();
				if (area < MIN_AREA) {
					MIN_AREA         = area;
					MIN_FOOTPRINT[0] = footprint[0];
					MIN_FOOTPRINT[1] = footprint[1];
					memcpy(BEST_BOARD, board, sizeof(ibrd));
				}
				m.unlock();
			}

			/* if area is less than best area */
		} else if (area < MIN_AREA) {
			int val = outer_step({cells[id].next, footprint, board, cells}).get();
			nnc.fetch_add(val);
			/* if area is greater than or equal to best area, prune search */
		} else {
			std::stringstream ss;
			ss << "T " << area << ", " << MIN_AREA << "\n";
			inncabs::debug(ss.str());
		}
	}
	return 0;
}

template<typename O, typename I>
int add_cell(const params& p, const O& outer_step, const I& inner_step) {
	int id = p.id;
	coor& FOOTPRINT = p.FOOTPRINT;
	ibrd& BOARD = p.BOARD;
	struct cell *CELLS = p.CELLS;

	int  i, j, nn, nnl;

	coor NWS[DMAX];

	nnl = 0;
	std::atomic_int_least32_t nnc { 0 };

    std::vector<decltype(allscale::api::core::run(inner_step(*(inner_params*)nullptr)))> futures;

	/* for each possible shape */
	for (i = 0; i < CELLS[id].n; i++) {
		/* compute all possible locations for nw corner */
		nn = starts(id, i, NWS, CELLS);
		nnl += nn;
		/* for all possible locations */
		for (j = 0; j < nn; j++) {
			futures.push_back(inner_step({nnc, CELLS, NWS, BOARD, FOOTPRINT, i, j, id}));
		}
	}
	for(auto& f : futures) {
		f.get();
	}
	return nnc + nnl;
}

ibrd board;

void floorplan_init(const char *filename) {
	inputFile = fopen(filename, "r");

	if(NULL == inputFile) {
		std::stringstream ss;
		ss << "Couldn't open file " << filename << " for reading\n";
		inncabs::error(ss.str());
	}

	/* read input file and initialize global minimum area */
	read_inputs();
	MIN_AREA = ROWS * COLS;

	/* initialize board is empty */
	for(int i = 0; i < ROWS; i++) {
		for(int j = 0; j < COLS; j++) {
			board[i][j] = 0;
		}
	}
}

void compute_floorplan(const std::launch l) {
	coor footprint;
	/* footprint of initial board is zero */
	footprint[0] = 0;
	footprint[1] = 0;
	inncabs::message("Computing floorplan ");

	auto prec_op = allscale::api::core::prec(
		allscale::api::core::fun( // outer
			[](const params& p){ return p.CELLS[p.id].n == 0; },
			[](const params& p){ return 0; },
			[](const params& p, const auto& outer_step, const auto& inner_step){ return add_cell(p, outer_step, inner_step); }
		),
		allscale::api::core::fun( // inner
			[](const inner_params& p){ return false; },
			[](const inner_params& p){ return 0; },
			[](const inner_params& p, const auto& outer_step, const auto& inner_step){ return add_cell_inner(p, outer_step, inner_step); }
		)
	);

	prec_op({1, footprint, board, gcells}).get();

	inncabs::message(" completed!\n");

}

void floorplan_end() {
	/* write results */
	write_outputs();
}

bool floorplan_verify() {
	if(solution != -1 ) return MIN_AREA == solution;
	return false;
}
