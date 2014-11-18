#include "../include/inncabs.h"

/* 
 * Based on the Barcelona OpenMP Tasks Suite "alignment" benchmark
 * Original code from the Application Kernel Matrix by Cray, that was based on the ClustalW application 
 */

#include <cmath>

#include "param.h"
#include "globals.h"
#include "sequence.h"
#include "alignment.h"

int main(int argc, char** argv) {
	std::string name = pairalign_init(argc > 1 ? argv[1] : "input/alignment/prot.20.aa");
	align_seq_init();
	align_seq();

	inncabs::run_all(
		[](const std::launch l) {
			return pairalign(l);
		},
		[](int result) {
			return align_verify(); 
		},
		name,
		[]() { align_init(); }
		);
}
