#pragma once

#define EOS '\0'
#define MAXLINE 512

#define NUMRES       32	  /* max size of comparison matrix */
#define MAXNAMES     30	  /* max chars read for seq. names */
#define FILENAMELEN 256   /* max file name length */

int ktup, window, signif;
int prot_ktup, prot_window, prot_signif;

int gap_pos1, gap_pos2, mat_avscore;
int nseqs, max_aa;
#define MAX_ALN_LENGTH 5000

int *seqlen_array, def_aa_xref[NUMRES+1];

int *bench_output, *seq_output;

double gap_open,      gap_extend;
double prot_gap_open, prot_gap_extend;
double pw_go_penalty, pw_ge_penalty;
double prot_pw_go_penalty, prot_pw_ge_penalty;

char **args, **names, **seq_array;

int matrix[NUMRES][NUMRES];

double gap_open_scale;
double gap_extend_scale;

// dnaFlag default value is false
bool dnaFlag = false;

// clustalw default value is false
bool clustalw = false;

#define INT_SCALE 100

#define MIN(a,b) ((a)<(b)?(a):(b))
#define tbgap(k) ((k) <= 0 ? 0 : tb + gh * (k))
#define tegap(k) ((k) <= 0 ? 0 : te + gh * (k))
