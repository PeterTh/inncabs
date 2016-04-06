#pragma once

#include "parec/core.h"

/**********************************************************************************************/
/*  This program is part of the Barcelona OpenMP Tasks Suite                                  */
/*  Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de Supercomputacion  */
/*  Copyright (C) 2009 Universitat Politecnica de Catalunya                                   */
/**********************************************************************************************/
/*
* Copyright (c) 2007 The Unbalanced Tree Search (UTS) Project Team:
* -----------------------------------------------------------------
*
*  This file is part of the unbalanced tree search benchmark.  This
*  project is licensed under the MIT Open Source license.  See the LICENSE
*  file for copyright and licensing information.
*
*  UTS is a collaborative project between researchers at the University of
*  Maryland, the University of North Carolina at Chapel Hill, and the Ohio
*  State University.
*
* University of Maryland:
*   Chau-Wen Tseng(1)  <tseng at cs.umd.edu>
*
* University of North Carolina, Chapel Hill:
*   Jun Huan         <huan,
*   Jinze Liu         liu,
*   Stephen Olivier   olivier,
*   Jan Prins*        prins at cs.umd.edu>
*
* The Ohio State University:
*   James Dinan      <dinan,
*   Gerald Sabin      sabin,
*   P. Sadayappan*    saday at cse.ohio-state.edu>
*
* Supercomputing Research Center
*   D. Pryor
*
* (1) - indicates project PI
*
* UTS Recursive Depth-First Search (DFS) version developed by James Dinan
*
* Adapted for OpenMP 3.0 Task-based version by Stephen Olivier
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
*
*/

#include "brg_sha1.h"

#define UTS_VERSION "2.1"

/***********************************************************
 *  Tree node descriptor and statistics                    *
 ***********************************************************/

#define MAXNUMCHILDREN    100  // cap on children (BIN root is exempt)

struct node_t {
  int height;        // depth of this node in the tree
  int numChildren;   // number of children, -1 => not yet determined

  /* for RNG state associated with this node */
  struct state_t state;
};

typedef struct node_t Node;

/* Tree type
 *   Trees are generated using a Galton-Watson process, in
 *   which the branching factor of each node is a random
 *   variable.
 *
 *   The random variable can follow a binomial distribution
 *   or a geometric distribution.  Hybrid tree are
 *   generated with geometric distributions near the
 *   root and binomial distributions towards the leaves.
 */
/* Tree  parameters */
extern double     b_0;
extern int        rootId;
extern int        nonLeafBF;
extern double     nonLeafProb;

/* Benchmark parameters */
extern int    computeGranularity;
extern int    debug;
extern int    verbose;

/* Utility Functions */
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

int    uts_paramsToStr(char *strBuf, int ind);
void   uts_read_file(const char *file);
void   uts_print_params();

double rng_toProb(int n);

/* Common tree routines */
void   uts_initRoot(Node * root);
int    uts_numChildren(Node *parent);
int    uts_numChildren_bin(Node * parent);
int    uts_numChildren_geo(Node * parent);
int    uts_childType(Node *parent);

void uts_show_stats();
bool uts_check_result(unsigned long long ntasks);

////////////////////////////////////////////////////// IMPLEMENTATION

/***********************************************************
*  Global state                                           *
***********************************************************/
unsigned long long nLeaves = 0;
int maxTreeDepth = 0;
/***********************************************************
* Tree generation strategy is controlled via various      *
* parameters set from the command line.  The parameters   *
* and their default values are given below.               *
* Trees are generated using a Galton-Watson process, in   *
* which the branching factor of each node is a random     *
* variable.                                               *
*                                                         *
* The random variable follow a binomial distribution.     *
***********************************************************/
double b_0   = 4.0; // default branching factor at the root
int   rootId = 0;   // default seed for RNG state at root
/***********************************************************
*  The branching factor at the root is specified by b_0.
*  The branching factor below the root follows an
*     identical binomial distribution at all nodes.
*  A node has m children with prob q, or no children with
*     prob (1-q).  The expected branching factor is q * m.
*
*  Default parameter values
***********************************************************/
int    nonLeafBF   = 4;            // m
double nonLeafProb = 15.0 / 64.0;  // q
/***********************************************************
* compute granularity - number of rng evaluations per
* tree node
***********************************************************/
int computeGranularity = 1;
unsigned long long number_of_tasks = 0;
/***********************************************************
* expected results for execution
***********************************************************/
unsigned long long exp_tree_size = 0;
int	exp_tree_depth = 0;
unsigned long long exp_num_leaves = 0;
/***********************************************************
*  FUNCTIONS                                              *
***********************************************************/

// Interpret 32 bit positive integer as value on [0,1)
double rng_toProb(int n) {
	if(n < 0) {
		printf("*** toProb: rand n = %d out of range\n",n);
	}
	return ((n<0)? 0.0 : ((double) n)/2147483648.0);
}

void uts_initRoot(Node *root) {
	root->height = 0;
	root->numChildren = -1;      // means not yet determined
	rng_init(root->state.state, rootId);

	std::stringstream ss;
	ss << "Root node at " << root << std::endl;
	inncabs::message(ss.str());
}


int uts_numChildren_bin(Node *parent) {
	// distribution is identical everywhere below root
	int    v = rng_rand(parent->state.state);
	double d = rng_toProb(v);

	return (d < nonLeafProb) ? nonLeafBF : 0;
}

int uts_numChildren(Node *parent) {
	int numChildren = 0;

	/* Determine the number of children */
	if(parent->height == 0) numChildren = (int) floor(b_0);
	else numChildren = uts_numChildren_bin(parent);

	// limit number of children
	// only a BIN root can have more than MAXNUMCHILDREN
	if(parent->height == 0) {
		int rootBF = (int) ceil(b_0);
		if(numChildren > rootBF) {
			numChildren = rootBF;
		}
	}
	else {
		if(numChildren > MAXNUMCHILDREN) {
			numChildren = MAXNUMCHILDREN;
		}
	}

	return numChildren;
}

/***********************************************************
* Recursive depth-first implementation                    *
***********************************************************/

struct params {
	int depth;
	Node *parent;
	int numChildren;
};

template<typename T>
unsigned long long parTreeSearch(params p, const T& rec_call) {
	int depth = p.depth;
	Node *parent = p.parent;
	int numChildren = p.numChildren;

	Node *n;
	n = (Node*)alloca(sizeof(Node)*numChildren);
	Node *nodePtr;
	unsigned long long subtreesize = 1;
	std::vector<decltype(rec_call(p))> futures;

	// Recurse on the children
	for(int i = 0; i < numChildren; i++) {
		nodePtr = &n[i];

		nodePtr->height = parent->height + 1;

		// The following line is the work (one or more SHA-1 ops)
		for(int j = 0; j < computeGranularity; j++) {
			rng_spawn(parent->state.state, nodePtr->state.state, i);
		}

		nodePtr->numChildren = uts_numChildren(nodePtr);

		futures.push_back( rec_call({depth+1, nodePtr, nodePtr->numChildren}) );
	}

	for(auto& f: futures) {
		subtreesize += f.get();
	}

	return subtreesize;
}

unsigned long long parallel_uts(const std::launch l, Node *root) {
	root->numChildren = uts_numChildren(root);

	auto p_uts = parec::prec(
		[](params p) { return p.numChildren == 0; },
		[](params p) { return 1ull; },
		[](params p, const auto& rec_call) { return parTreeSearch(p, rec_call); }
	);

	return p_uts({0, root, root->numChildren}).get();
}

void uts_read_file(const char *filename) {
	FILE *fin = fopen(filename, "r");

	if(fin == NULL) {
		std::stringstream ss;
		ss << "Could not open input file (" << filename << ")\n";
		inncabs::error(ss.str());
	}
	fscanf(fin,"%lf %lf %d %d %d %llu %d %llu",
		&b_0,
		&nonLeafProb,
		&nonLeafBF,
		&rootId,
		&computeGranularity,
		&exp_tree_size,
		&exp_tree_depth,
		&exp_num_leaves
		);
	fclose(fin);

	computeGranularity = max(1,computeGranularity);

	// Printing input data
	std::stringstream ss;
	ss << "\n";
	ss << "Root branching factor                = " << b_0 << "\n";
	ss << "Root seed (0 <= 2^31)                = " << rootId << "\n";
	ss << "Probability of non-leaf node         = " << nonLeafProb << "\n";
	ss << "Number of children for non-leaf node = " << nonLeafBF << "\n";
	ss << "E(n)                                 = " << (double) ( nonLeafProb * nonLeafBF ) << "\n";
	ss << "E(s)                                 = " << (double) ( 1.0 / (1.0 - nonLeafProb * nonLeafBF) );
	ss << "Compute granularity                  = " << computeGranularity << "\n";
	ss << "Random number generator              = "; rng_showtype();
}

void uts_show_stats() {
	int chunkSize = 0;

	std::stringstream ss;
	ss << "\n";
	ss << "Tree size                            = " << (unsigned long long)number_of_tasks << "\n";
	ss << "Maximum tree depth                   = " << maxTreeDepth << "\n";
	ss << "Chunk size                           = " << chunkSize << "\n";
	ss << "Number of leaves                     = " << nLeaves << " (" << nLeaves/(float)number_of_tasks*100.0 << "%)\n";
	inncabs::message(ss.str());
}

bool uts_check_result(unsigned long long ntasks) {
	bool answer = true;

	if(ntasks != exp_tree_size ) {
		answer = false;
		std::stringstream ss;
		ss << "Incorrect tree size result (" << ntasks << " instead of " << exp_tree_size << ").\n";
		inncabs::message(ss.str());
	}

	return answer;
}
