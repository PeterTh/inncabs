#include "../include/inncabs.h"

#include <fstream>
#include <utility>
#include <cassert>
#include <set>
#include <queue>

#define MAX_PORTS 3

using namespace std;

// some type aliases
typedef char Symbol;

class Cell;

// a type forming half a wire connection
struct Port {
	Cell* cell;
	unsigned port;
	Port(Cell* cell = 0, unsigned port = 0)
	 : cell(cell), port(port) {}

	operator Cell*() const { return cell; }
	Cell* operator->() const { return cell; }
};

/**
 * An abstract base class for all cells.
 */
class Cell {
	Symbol symbol;
	unsigned numPorts;
	Port ports[MAX_PORTS];
	std::mutex lock;
	bool alive;

public:
	Cell(Symbol symbol, unsigned numPorts) 
		: symbol(symbol), numPorts(numPorts), alive(true) { 
		assert(numPorts > 0 && numPorts <= MAX_PORTS);
		for(unsigned i=0; i<numPorts; i++) {
			ports[i] = Port();
		}
	}

	virtual ~Cell() {};

	void die() {
		alive = false;
	}

	bool dead() {
		return !alive;
	}

	Symbol getSymbol() const { return symbol; }

	unsigned getNumPorts() const { return numPorts; }

	const Port& getPrinciplePort() const {
		return getPort(0);
	}

	void setPrinciplePort(const Port& wire) {
		setPort(0, wire);	
	}

	const Port& getPort(unsigned index) const {
		assert(index < numPorts);
		return ports[index];
	}

	void setPort(unsigned index, const Port& wire) {
		assert(index < numPorts);
		ports[index] = wire;
	}

	std::mutex& getLock() {
		return lock;
	}
};


void link(Cell* a, unsigned portA, Cell* b, unsigned portB) {
	a->setPort(portA, Port(b, portB));
	b->setPort(portB, Port(a, portA));
}

void link(Cell* a, unsigned portA, const Port& portB) {
	link(a, portA, portB.cell, portB.port);
}

void link(const Port& portA, Cell* b, unsigned portB) {
	link(portA.cell, portA.port, b, portB);
}

void link(const Port& a, const Port& b) {
	link(a.cell, a.port, b);
}


// ------ Special Cell Nodes ---------


struct End : public Cell {
	End() : Cell('#',1) {}
};

struct Zero : public Cell {
	Zero() : Cell('0',1) {}
};

struct Succ : public Cell {
	Succ() : Cell('s',2) {}
};

struct Add : public Cell {
	Add() : Cell('+',3) {}
};

struct Mul : public Cell {
	Mul() : Cell('*',3) {}
};

struct Eraser : public Cell {
	Eraser() : Cell('e', 1) {}
};

struct Duplicator : public Cell {
	Duplicator() : Cell('d', 3) {}
};


// ------- Net Construction Operations -------

Cell* toNet(unsigned value) {

	// terminal case
	if (value == 0) return new Zero();

	// other cases
	auto inner = toNet(value - 1);
	auto succ = new Succ();
	link(succ, 1, inner, 0);
	return succ;
}

int toValue(Cell* net) {
	// compute value
	if (net->getSymbol() == '0') return 0;
	if (net->getSymbol() == 's') {
		int res = toValue(net->getPort(1));
		return (res == -1) ? -1 : res + 1;
	}
	return -1;
}

Port add(Port a, Port b) {
	auto res = new Add();
	link(res, 0, a);
	link(res, 2, b);
	return Port(res, 1);
}

Port mul(Port a, Port b) {
	auto res = new Mul();
	link(res, 0, a);
	link(res, 2, b);
	return Port(res, 1);
}

Cell* end(Port a) {
	auto res = new End();
	link(res, 0, a);
	return res;
}

// ------- Computation Operation --------

void compute(Cell* net);


// other utilities
void destroy(const Cell* net);
void plotGraph(const Cell* cell, const string& filename = "net.dot");

// -----------------------------------------------------------------------

namespace {
	void collectClosure(const Cell* cell, set<const Cell*>& res) {
		if (!cell) return;

		// add current cell to resulting set
		bool newElement = res.insert(cell).second;
		if (!newElement) return;

		// continue with connected ports
		for(unsigned i=0; i<cell->getNumPorts(); i++) {
			collectClosure(cell->getPort(i), res);
		}
	}
}

set<const Cell*> getClosure(const Cell* cell) {
	set<const Cell*> res;
	collectClosure(cell, res);
	return res;
}

void plotGraph(Cell* cell, const string& filename) {

	// step 0: open file
	ofstream file;
	file.open(filename);

	// step 1: get set of all cells (convex closure)
	auto cells = getClosure(cell);

	// step 2: print dot header
	file << "digraph test {\n";

	// step 3: print node and edge description
	for(const Cell* cur : cells) {
		// add node description
		file << "\tn" << (cur) << " [label=\"" << cur->getSymbol() << "\"];\n";

		// add ports
		for(unsigned i=0; i<cur->getNumPorts(); i++) {
			const Cell* other = cur->getPort(i).cell;

			if (other) {
				file << "\tn" << (cur) << " -> n" << (other) 
					<< " [label=\"\"" << ((i==0)?", color=\"blue\"":"") << "];\n";
			}
		}
	}

	// step 4: finish description
	file << "}\n";

	// step 5: close file
	file.close();
}

void destroy(const Cell* net) {

	// step 1: get all cells
	auto cells = getClosure(net);

	// step 2: destroy them
	for(const Cell* cur : cells) {
		delete cur;
	}
}

namespace {

	bool isCut(const Cell* a, const Cell* b) {
		return a->getPrinciplePort().cell == b && b->getPrinciplePort().cell == a;
	}

	template<typename T>
	void unlockAll(T& head) {
		head.unlock();
	}
	template<typename T, typename... Types>
	void unlockAll(T& head, Types&... args) {
		head.unlock();
		unlockAll(args...);
	}
}


void handleCell(const std::launch l, Cell* a) {
	Cell * b = a->getPrinciplePort().cell;
	std::lock(a->getLock(), b->getLock());

	Symbol aSym = a->getSymbol();
	Symbol bSym = b->getSymbol();

	// ensure proper order
	if(aSym < bSym) {
		unlockAll(a->getLock(), b->getLock());
		handleCell(l, b);
		return;
	}

	/*std::cout << "starting on " << a << ", " << b << std::endl;*/

	Cell *x = a->getNumPorts()>1 ? a->getPort(1).cell : nullptr;
	Cell *y = a->getNumPorts()>2 ? a->getPort(2).cell : nullptr;
	Cell *z = b->getNumPorts()>1 ? b->getPort(1).cell : nullptr;
	Cell *w = b->getNumPorts()>2 ? b->getPort(2).cell : nullptr;

	std::mutex xLT, yLT, zLT, wLT;
	std::mutex* aLock = &a->getLock();
	std::mutex* bLock = &b->getLock();
	std::mutex* xLock = x ? &x->getLock() : &xLT;
	std::mutex* yLock = y ? &y->getLock() : &yLT;
	std::mutex* zLock = z ? &z->getLock() : &zLT;
	std::mutex* wLock = w ? &w->getLock() : &wLT;
	
	unlockAll(*aLock, *bLock);
	std::lock(*aLock, *bLock, *xLock, *yLock, *zLock, *wLock);

	if(    a->getPrinciplePort().cell != b 
		|| a->dead() || b->dead() 
		|| (x && x->dead()) || (y && y->dead()) || (z && z->dead()) || (w && w->dead())
		|| x != (a->getNumPorts()>1 ? a->getPort(1).cell : nullptr)
		|| y != (a->getNumPorts()>2 ? a->getPort(2).cell : nullptr)
		|| z != (b->getNumPorts()>1 ? b->getPort(1).cell : nullptr)
		|| w != (b->getNumPorts()>2 ? b->getPort(2).cell : nullptr) 
		|| aSym != a->getSymbol()
		|| bSym != b->getSymbol()) {
		// need to retry later
		unlockAll(*aLock, *bLock, *xLock, *yLock, *zLock, *wLock);
		if(!a->dead()) handleCell(l, a);
		return;
	}

	/*std::cout << "fully locked on " << a << ", " << b << std::endl;*/ 
	
	/*
		// debugging:
		stringstream file;
		file << "step" << counter++ << ".dot";
		plotGraph(net, file.str());
		std::cout << "Step: " << counter << " - Processing: " << a->getSymbol() << " vs. " << b->getSymbol() << "\n";
	*/
	std::vector<Cell*> newTasks;
	switch(a->getSymbol()) {
		case '0': {
			switch(b->getSymbol()) {
			case '+': {
				// implement the 0+ rule
				auto x = b->getPort(1);
				auto y = b->getPort(2);

				link(x, y);

				a->die();
				b->die();

				// check whether this is producing a cut
				if(isCut(x, y)) newTasks.push_back(x);

				break;
			}
			case '*': {
				// implement s* rule
				auto x = b->getPort(1);
				auto y = b->getPort(2);

				// create a new cell
				auto e = new Eraser();

				// alter wiring
				link(a, 0, x);
				link(e, 0, y);
				
				// eliminate nodes
				b->die();

				// check for new cuts
				if(isCut(a, x)) newTasks.push_back(x);
				if(isCut(e, y)) newTasks.push_back(y);

				break;
			}
			case 'd': {
				// implement the 0+ rule
				auto x = b->getPort(1);
				auto y = b->getPort(2);
				
				// creat new cell
				auto n = new Zero();

				// update links
				link(a, 0, b, 2);
				link(n, 0, b, 1);

				// remove old cell
				b->die();

				// check whether this is producing a cut
				if(isCut(a, y)) newTasks.push_back(y);
				if(isCut(n, x)) newTasks.push_back(x);

				break;
			}
			default: break;
			}
			break;
		}
		case 's': {
			switch(b->getSymbol()) {
			case '+': {
				// implement the s+ rule
				auto x = a->getPort(1);
				auto y = b->getPort(1);
				
				// alter wiring
				link(x, b, 0);
				link(y, a, 0);
				link(a,1,b,1);

				// check for new cuts
				if(isCut(x, b)) newTasks.push_back(x);
				if(isCut(y, a)) newTasks.push_back(y);

				break;
			}
			case '*': {	
				// implement s* rule
				auto x = a->getPort(1);
				auto y = b->getPort(1);
				auto z = b->getPort(2);
				
				// create new cells
				auto p = new Add();
				auto d = new Duplicator();

				// alter wiring
				link(b, 0, x);
				link(b, 1, p, 0);
				link(b, 2, d, 1);

				link(p, 1, y);
				link(p, 2, d, 2);

				link(d, 0, z);
				
				// eliminate nodes
				a->die();

				// check for new 
				if(isCut(x, b)) newTasks.push_back(x);
				if(isCut(d, z)) newTasks.push_back(d);

				break;
			}
			case 'd': {
				// implement the sd rule
				auto x = a->getPort(1);
				auto y = b->getPort(1);
				auto z = b->getPort(2);
				
				// crate new cells
				auto s = new Succ();

				// alter wiring
				link(b, 0, x);
				link(b, 1, s, 1);
				link(b, 2, a, 1);

				link(s, 0, z);
				link(a, 0, y);

				// check for new cuts
				if(isCut(x, b)) newTasks.push_back(x);
				if(isCut(y, a)) newTasks.push_back(y);
				if(isCut(z, s)) newTasks.push_back(z);

				break;
			}
			case 'e': {
				// implement the sd rule
				auto x = a->getPort(1);
				
				// alter wiring
				link(b, 0, x);

				// delete old cell
				a->die();

				// check for new cuts
				if(isCut(x, b)) newTasks.push_back(x);

				break;
			}
			default: break;
			}
			break;
		}
		case '+': break;
		case 'd': {
			switch(b->getSymbol()) {
			case '0': {
				// implement the 0+ rule
				auto x = a->getPort(1);
				auto y = a->getPort(2);
				
				// creat new cell
				auto n = new Zero();

				// update links
				link(b, 0, x);
				link(n, 0, y);

				// remove old cell
				a->die();

				// check whether this is producing a cut
				if(isCut(b, x)) newTasks.push_back(x);
				if(isCut(n, y)) newTasks.push_back(y);

				break;
			}
			default: break;
			}
			break;
		}
		case 'e': {
			switch(b->getSymbol()) {
			case '0': {
				// just delete cells
				a->die();
				b->die();
				break;
			}
			default: break;
			}
			break;
		}
		default: break;
	}

	/*std::cout << "pre-unlocked on " << a << ", " << b << " locks: " <<  *(int*)&a->getLock() << " / " <<  *(int*)&b->getLock() << std::endl;*/
	unlockAll(*aLock, *bLock, *xLock, *yLock, *zLock, *wLock);
	/*std::cout << "unlocked on " << a << ", " << b << " locks: " <<  *(int*)&a->getLock() << " / " <<  *(int*)&b->getLock() << std::endl;*/  

	std::vector<std::future<void>> futures;
	for(Cell* c : newTasks) futures.push_back(inncabs::async(l, &handleCell, l, c));
	for(auto& f : futures) f.wait();
}


void compute(const std::launch l, Cell* net) {

	// step 1: get all cells in the net
	set<const Cell*> cells = getClosure(net);

	// step 2: get all connected principle ports
	std::vector<std::future<void>> cuts;
	for(const Cell* cur : cells) {
		const Port& port = cur->getPrinciplePort();
		if(cur < port.cell && isCut(cur, port.cell)) {
			cuts.push_back(inncabs::async(l, handleCell, l, const_cast<Cell*>(cur)));
		}
	}

//std::cout << "Found " << cuts.size() << " cut(s).\n";
	
	// step 3: run processing
	for(auto& f: cuts) {
		f.wait();
	}

/*
	// debugging:
	stringstream file;
	file << "step" << counter++ << ".dot";
	plotGraph(net, file.str());
*/
}


int main(int argc, char** argv) {
	int N = 500;
	if(argc > 1) N = std::stoi(argv[1]);

	std::stringstream ss;
	ss << "Intersim (N = " << N << ")";
	Cell* n;
	inncabs::run_all(
		[&](const std::launch l) {
			compute(l, n);
			return n;
		},
		[&](Cell* result) {
			bool ret = toValue(result->getPort(0)) == N*N;
			// clean up remaining network
			destroy(result);
			return ret;
		},
		ss.str(),
		[&]{ n = end(mul(toNet(N), toNet(N))); }
		);

	return 0;
}
