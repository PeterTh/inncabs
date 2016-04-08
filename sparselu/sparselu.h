#pragma once

#define EPSILON 1.0E-6

int arg_size_1, arg_size_2;
float **SEQ,**BENCH;

bool checkmat(float *M, float *N);
void genmat(float *M[]);
void print_structure(const char *name, float *M[]);
float* allocate_clean_block();
void lu0(float *diag);
void bdiv(float *diag, float *row);
void bmod(float *row, float *col, float *inner);
void fwd(float *diag, float *col);

void sparselu_init(float ***pBENCH, const char *pass); 
void sparselu(float **BENCH);
void sparselu_fini(float **BENCH, const char *pass); 

void sparselu_seq_call(float **BENCH);
void sparselu_par_call(const std::launch t, float **BENCH);

bool sparselu_check(float **SEQ, float **BENCH);

///////////////////////////////////////////////// IMPLEMENTATION

#include <vector>
#include <cilk/cilk.h>

#define TRUE  1
#define FALSE 0

bool checkmat(float *M, float *N) {
	float r_err;

	for(int i = 0; i < arg_size_2; i++) {
		for(int j = 0; j < arg_size_2; j++) {
			r_err = M[i*arg_size_2+j] - N[i*arg_size_2+j];
			if(r_err == 0.0) continue;

			if(r_err < 0.0) r_err = -r_err;

			if(M[i*arg_size_2+j] == 0) {
				std::stringstream ss;
				ss << "Checking failure: A[" << i << "][" << j << "]=" << M[i*arg_size_2+j] 
				<< "  B[" << i << "][" << j << "]=" << N[i*arg_size_2+j] << "; \n";
				inncabs::message(ss.str());
				return false;
			}  
			r_err = r_err / M[i*arg_size_2+j];
			if(r_err > EPSILON) {
				std::stringstream ss;
				ss << "Checking failure: A[" << i << "][" << j << "]=" << M[i*arg_size_2+j] 
				<< "  B[" << i << "][" << j << "]=" << N[i*arg_size_2+j] << "; \n"
					<< "; Relative Error=" << r_err << "\n";
				inncabs::message(ss.str());
				return false;
			}
		}
	}
	return true;
}

void genmat(float *M[]) {
	int null_entry, init_val, i, j, ii, jj;
	float *p;

	init_val = 1325;

	/* generating the structure */
	for(ii=0; ii < arg_size_1; ii++) {
		for(jj=0; jj < arg_size_1; jj++) {
			/* computing null entries */
			null_entry = FALSE;
			if((ii<jj) && (ii%3 !=0)) null_entry = TRUE;
			if((ii>jj) && (jj%3 !=0)) null_entry = TRUE;
			if(ii%2==1) null_entry = TRUE;
			if(jj%2==1) null_entry = TRUE;
			if(ii==jj) null_entry = FALSE;
			if(ii==jj-1) null_entry = FALSE;
			if(ii-1 == jj) null_entry = FALSE; 
			/* allocating matrix */
			if(null_entry == FALSE){
				M[ii*arg_size_1+jj] = (float *) malloc(arg_size_2*arg_size_2*sizeof(float));
				if(M[ii*arg_size_1+jj] == NULL) {
					inncabs::error("Error: Out of memory\n");
				}
				/* initializing matrix */
				p = M[ii*arg_size_1+jj];
				for(i = 0; i < arg_size_2; i++) {
					for(j = 0; j < arg_size_2; j++) {
						init_val = (3125 * init_val) % 65536;
						(*p) = (float)((init_val - 32768.0) / 16384.0);
						p++;
					}
				}
			}
			else {
				M[ii*arg_size_1+jj] = NULL;
			}
		}
	}
}

void print_structure(const char *name, float *M[]) {
	std::stringstream ss;
	ss << "Structure for matrix " << name << " @ 0x" << M << "\n";
	for(int ii = 0; ii < arg_size_1; ii++) {
		for(int jj = 0; jj < arg_size_1; jj++) {
			if(M[ii*arg_size_1+jj]!=NULL) ss << "x";
			else ss << " ";
		}
		ss << "\n";
	}
	ss << "\n";
	inncabs::message(ss.str());
}

float* allocate_clean_block() {
	int i,j;
	float *p, *q;

	p = (float *) malloc(arg_size_2*arg_size_2*sizeof(float));
	q = p;
	if(p!=NULL) {
		for(i = 0; i < arg_size_2; i++) {
			for(j = 0; j < arg_size_2; j++) { 
				(*p)=0.0; 
				p++;
			}
		}
	}
	else {
		inncabs::error("Error: Out of memory\n");
	}
	return (q);
}

void lu0(float *diag) {
	for(int k=0; k<arg_size_2; k++) {
		for(int i=k+1; i<arg_size_2; i++) {
			diag[i*arg_size_2+k] = diag[i*arg_size_2+k] / diag[k*arg_size_2+k];
			for(int j=k+1; j<arg_size_2; j++)
				diag[i*arg_size_2+j] = diag[i*arg_size_2+j] - diag[i*arg_size_2+k] * diag[k*arg_size_2+j];
		}
	}
}

void bdiv(float *diag, float *row) {
	for(int i=0; i<arg_size_2; i++) {
		for(int k=0; k<arg_size_2; k++) {
			row[i*arg_size_2+k] = row[i*arg_size_2+k] / diag[k*arg_size_2+k];
			for(int j=k+1; j<arg_size_2; j++)
				row[i*arg_size_2+j] = row[i*arg_size_2+j] - row[i*arg_size_2+k]*diag[k*arg_size_2+j];
		}
	}
}

void bmod(float *row, float *col, float *inner) {
	for(int i=0; i<arg_size_2; i++)
		for(int j=0; j<arg_size_2; j++)
			for(int k=0; k<arg_size_2; k++)
				inner[i*arg_size_2+j] = inner[i*arg_size_2+j] - row[i*arg_size_2+k]*col[k*arg_size_2+j];
}

void fwd(float *diag, float *col) {
	for(int j=0; j<arg_size_2; j++)
		for(int k=0; k<arg_size_2; k++) 
			for(int i=k+1; i<arg_size_2; i++)
				col[i*arg_size_2+j] = col[i*arg_size_2+j] - diag[i*arg_size_2+k]*col[k*arg_size_2+j];
}


void sparselu_init(float ***pBENCH, const char *pass) {
	*pBENCH = (float **) malloc(arg_size_1*arg_size_1*sizeof(float *));
	genmat(*pBENCH);
	print_structure(pass, *pBENCH);
}

void sparselu_par_call(const std::launch l, float **BENCH) {
	for(int kk=0; kk<arg_size_1; kk++) {
		lu0(BENCH[kk*arg_size_1+kk]);
		for(int jj=kk+1; jj<arg_size_1; jj++) {
			if(BENCH[kk*arg_size_1+jj] != NULL) {
				cilk_spawn fwd(BENCH[kk*arg_size_1+kk], BENCH[kk*arg_size_1+jj]);
			}
			for(int ii=kk+1; ii<arg_size_1; ii++) {
				if(BENCH[ii*arg_size_1+kk] != NULL)	{
					cilk_spawn bdiv(BENCH[kk*arg_size_1+kk], BENCH[ii*arg_size_1+kk]);
				}
			}
			cilk_sync;

			for(int ii=kk+1; ii<arg_size_1; ii++) {
				if(BENCH[ii*arg_size_1+kk] != NULL) {
					for(jj=kk+1; jj<arg_size_1; jj++) {
						if(BENCH[kk*arg_size_1+jj] != NULL)	{
							cilk_spawn [=]() {
								if(BENCH[ii*arg_size_1+jj]==NULL) BENCH[ii*arg_size_1+jj] = allocate_clean_block();
								bmod(BENCH[ii*arg_size_1+kk], BENCH[kk*arg_size_1+jj], BENCH[ii*arg_size_1+jj]);
							} ();
						}
					}
				}
			}
			cilk_sync;
		}
	}
	inncabs::message("completed!\n");
}

void sparselu_seq_call(float **BENCH) {
	for(int kk=0; kk<arg_size_1; kk++) {
		lu0(BENCH[kk*arg_size_1+kk]);
		for(int jj=kk+1; jj<arg_size_1; jj++) {
			if(BENCH[kk*arg_size_1+jj] != NULL) {
				fwd(BENCH[kk*arg_size_1+kk], BENCH[kk*arg_size_1+jj]);
			}
			for(int ii=kk+1; ii<arg_size_1; ii++) {
				if(BENCH[ii*arg_size_1+kk] != NULL)	{
					bdiv(BENCH[kk*arg_size_1+kk], BENCH[ii*arg_size_1+kk]);
				}
			}
			for(int ii=kk+1; ii<arg_size_1; ii++) {
				if(BENCH[ii*arg_size_1+kk] != NULL) {
					for(jj=kk+1; jj<arg_size_1; jj++) {
						if(BENCH[kk*arg_size_1+jj] != NULL) {
							if(BENCH[ii*arg_size_1+jj]==NULL) BENCH[ii*arg_size_1+jj] = allocate_clean_block();
							bmod(BENCH[ii*arg_size_1+kk], BENCH[kk*arg_size_1+jj], BENCH[ii*arg_size_1+jj]);
						}
					}
				}
			}
		}
	}
}

void sparselu_fini(float **BENCH, const char *pass) {
	print_structure(pass, BENCH);
}

bool sparselu_check(float **SEQ, float **BENCH) {
	bool ok = true;

	for(int ii=0; ((ii<arg_size_1) && ok); ii++) {
		for(int jj=0; ((jj<arg_size_1) && ok); jj++) {
			if((SEQ[ii*arg_size_1+jj] == NULL) && (BENCH[ii*arg_size_1+jj] != NULL)) ok = false;
			if((SEQ[ii*arg_size_1+jj] != NULL) && (BENCH[ii*arg_size_1+jj] == NULL)) ok = false;
			if((SEQ[ii*arg_size_1+jj] != NULL) && (BENCH[ii*arg_size_1+jj] != NULL))
				ok = checkmat(SEQ[ii*arg_size_1+jj], BENCH[ii*arg_size_1+jj]);
		}
	}
	return ok;
}
