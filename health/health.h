#pragma once

/**********************************************************************************************/
/*  This program is part of the Barcelona OpenMP Tasks Suite                                  */
/*  Copyright (C) 2009 Barcelona Supercomputing Center - Centro Nacional de Supercomputacion  */
/*  Copyright (C) 2009 Universitat Politecnica de Catalunya                                   */
/**********************************************************************************************/

/* OLDEN parallel C for dynamic structures: compiler, runtime system
* and benchmarks
*
* Copyright (C) 1994-1996 by Anne Rogers (amr@cs.princeton.edu) and
* Martin Carlisle (mcc@cs.princeton.edu)
* ALL RIGHTS RESERVED.
*
* OLDEN is distributed under the following conditions:
*
* You may make copies of OLDEN for your own use and modify those copies.
*
* All copies of OLDEN must retain our names and copyright notice.
*
* You may not sell OLDEN or distribute OLDEN in conjunction with a
* commercial product or service without the expressed written consent of
* Anne Rogers and Martin Carlisle.
*
* THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE.
*
*/

#include <mutex>

#include "allscale/api/core/prec.h"

/* random defines */
#define IA 16807
#define IM 2147483647
#define AM (1.0 / IM)
#define IQ 127773
#define IR 2836
#define MASK 123459876

struct Results {
   long hosps_number;
   long hosps_personnel;
   long total_patients;
   long total_in_village;
   long total_waiting;
   long total_assess;
   long total_inside;
   long total_time;
   long total_hosps_v;
};

extern int sim_level;

struct Patient {
   int id;
   int32_t seed;
   int time;
   int time_left;
   int hosps_visited;
   struct Village *home_village;
   struct Patient *back;
   struct Patient *forward;
};
struct Hosp {
   int personnel;
   int free_personnel;
   struct Patient *waiting;
   struct Patient *assess;
   struct Patient *inside;
   struct Patient *realloc;
   std::mutex realloc_lock;
};
struct Village {
   int id;
   struct Village *back;
   struct Village *next;
   struct Village *forward;
   struct Patient *population;
   struct Hosp hosp;
   int level;
   int32_t  seed;
};

float my_rand(int32_t *seed);

struct Patient *generate_patient(struct Village *village);
void put_in_hosp(struct Hosp *hosp, struct Patient *patient);

void addList(struct Patient **list, struct Patient *patient);
void removeList(struct Patient **list, struct Patient *patient);

void check_patients_inside(struct Village *village);
void check_patients_waiting(struct Village *village);
void check_patients_realloc(struct Village *village);

void check_patients_assess_par(struct Village *village);

float get_num_people(struct Village *village);
float get_total_time(struct Village *village);
float get_total_hosps(struct Village *village);

struct Results get_results(struct Village *village);

void read_input_data(char *filename);
void allocate_village(struct Village **capital, struct Village *back, struct Village *next, int level, int32_t vid);
void sim_village_main_par(const std::launch l, struct Village *top);

void sim_village_par(const std::launch l, struct Village *village);
bool check_village(struct Village *top);

void check_patients_assess(struct Village *village);
void check_patients_population(struct Village *village);
void sim_village(struct Village *village);
void my_print(struct Village *village);

///////////////////////////////////////////////// IMPLEMENTATION

#include <vector>
#include <cmath>

/* global variables */
int sim_level;
int sim_cities;
int sim_population_ratio;
int sim_time;
int sim_assess_time;
int sim_convalescence_time;
int32_t sim_seed;
float sim_get_sick_p;
float sim_convalescence_p;
float sim_realloc_p;
int sim_pid = 0;

int res_population;
int res_hospitals;
int res_personnel;
int res_checkin;
int res_village;
int res_waiting;
int res_assess;
int res_inside;
float res_avg_stay;

float my_rand(int32_t *seed) {
	int32_t k;
	int32_t idum = *seed;

	idum ^= MASK;
	k = idum / IQ;
	idum = IA * (idum - k * IQ) - IR * k;
	idum ^= MASK;
	if (idum < 0) idum  += IM;
	*seed = idum * IM;
	return (float) AM * idum;
}

/********************************************************************
* Handles lists.                                                   *
********************************************************************/
void addList(struct Patient **list, struct Patient *patient) {
	if (*list == NULL)
	{
		*list = patient;
		patient->back = NULL;
		patient->forward = NULL;
	}
	else
	{
		struct Patient *aux = *list;
		while (aux->forward != NULL) aux = aux->forward;
		aux->forward = patient;
		patient->back = aux;
		patient->forward = NULL;
	}
}
void removeList(struct Patient **list, struct Patient *patient) {
	if (patient->back != NULL) patient->back->forward = patient->forward;
	else *list = patient->forward;
	if (patient->forward != NULL) patient->forward->back = patient->back;
}

/**********************************************************************/
void allocate_village( struct Village **capital, struct Village *back, struct Village *next, int level, int32_t vid) {
	int i, population, personnel;
	struct Village *current = NULL, *inext;
	struct Patient *patient;

	if(level == 0) *capital = NULL;
	else {
		personnel = (int) pow(2, level);
		population = personnel * sim_population_ratio;
		/* Allocate Village */
		*capital = new Village();//(struct Village *) malloc(sizeof(struct Village));
		/* Initialize Village */
		(*capital)->back  = back;
		(*capital)->next  = next;
		(*capital)->level = level;
		(*capital)->id    = vid;
		(*capital)->seed  = vid * (IQ + sim_seed);
		(*capital)->population = NULL;
		for(i=0;i<population;i++) {
			patient = (struct Patient *)malloc(sizeof(struct Patient));
			patient->id = sim_pid++;
			patient->seed = (*capital)->seed;
			// changes seed for capital:
			my_rand(&((*capital)->seed));
			patient->hosps_visited = 0;
			patient->time          = 0;
			patient->time_left     = 0;
			patient->home_village = *capital;
			addList(&((*capital)->population), patient);
		}
		/* Initialize Hospital */
		(*capital)->hosp.personnel = personnel;
		(*capital)->hosp.free_personnel = personnel;
		(*capital)->hosp.assess = NULL;
		(*capital)->hosp.waiting = NULL;
		(*capital)->hosp.inside = NULL;
		(*capital)->hosp.realloc = NULL;

		// Create Cities (lower level)
		inext = NULL;
		for (i = sim_cities; i>0; i--)
		{
			allocate_village(&current, *capital, inext, level-1, (vid * (int32_t) sim_cities)+ (int32_t) i);
			inext = current;
		}
		(*capital)->forward = current;
	}
}
/**********************************************************************/
struct Results get_results(struct Village *village)
{
	struct Village *vlist;
	struct Patient *p;
	struct Results t_res, p_res;

	t_res.hosps_number     = 0.0;
	t_res.hosps_personnel  = 0.0;
	t_res.total_patients   = 0.0;
	t_res.total_in_village = 0.0;
	t_res.total_waiting    = 0.0;
	t_res.total_assess     = 0.0;
	t_res.total_inside     = 0.0;
	t_res.total_hosps_v    = 0.0;
	t_res.total_time       = 0.0;

	if (village == NULL) return t_res;

	/* Traverse village hierarchy (lower level first)*/
	vlist = village->forward;
	while(vlist)
	{
		p_res = get_results(vlist);
		t_res.hosps_number     += p_res.hosps_number;
		t_res.hosps_personnel  += p_res.hosps_personnel;
		t_res.total_patients   += p_res.total_patients;
		t_res.total_in_village += p_res.total_in_village;
		t_res.total_waiting    += p_res.total_waiting;
		t_res.total_assess     += p_res.total_assess;
		t_res.total_inside     += p_res.total_inside;
		t_res.total_hosps_v    += p_res.total_hosps_v;
		t_res.total_time       += p_res.total_time;
		vlist = vlist->next;
	}
	t_res.hosps_number     += 1;
	t_res.hosps_personnel  += village->hosp.personnel;

	// Patients in the village
	p = village->population;
	while (p != NULL)
	{
		t_res.total_patients   += 1;
		t_res.total_in_village += 1;
		t_res.total_hosps_v    += p->hosps_visited;
		t_res.total_time       += p->time;
		p = p->forward;
	}
	// Patients in hospital: waiting
	p = village->hosp.waiting;
	while (p != NULL)
	{
		t_res.total_patients += 1;
		t_res.total_waiting  += 1;
		t_res.total_hosps_v  += p->hosps_visited;
		t_res.total_time     += p->time;
		p = p->forward;
	}
	// Patients in hospital: assess
	p = village->hosp.assess;
	while (p != NULL)
	{
		t_res.total_patients += 1;
		t_res.total_assess   += 1;
		t_res.total_hosps_v  += p->hosps_visited;
		t_res.total_time     += p->time;
		p = p->forward;
	}
	// Patients in hospital: inside
	p = village->hosp.inside;
	while (p != NULL)
	{
		t_res.total_patients += 1;
		t_res.total_inside   += 1;
		t_res.total_hosps_v  += p->hosps_visited;
		t_res.total_time     += p->time;
		p = p->forward;
	}

	return t_res;
}
/**********************************************************************/
/**********************************************************************/
/**********************************************************************/
void check_patients_inside(struct Village *village)
{
	struct Patient *list = village->hosp.inside;
	struct Patient *p;

	while (list != NULL)
	{
		p = list;
		list = list->forward;
		p->time_left--;
		if (p->time_left == 0)
		{
			village->hosp.free_personnel++;
			removeList(&(village->hosp.inside), p);
			addList(&(village->population), p);
		}
	}
}
/**********************************************************************/
void check_patients_assess_par(struct Village *village)
{
	struct Patient *list = village->hosp.assess;
	float rand;
	struct Patient *p;

	while (list != NULL)
	{
		p = list;
		list = list->forward;
		p->time_left--;

		if (p->time_left == 0)
		{
			rand = my_rand(&(p->seed));
			/* sim_covalescense_p % */
			if (rand < sim_convalescence_p)
			{
				rand = my_rand(&(p->seed));
				/* !sim_realloc_p % or root hospital */
				if (rand > sim_realloc_p || village->level == sim_level)
				{
					removeList(&(village->hosp.assess), p);
					addList(&(village->hosp.inside), p);
					p->time_left = sim_convalescence_time;
					p->time += p->time_left;
				}
				else /* move to upper level hospital !!! */
				{
					village->hosp.free_personnel++;
					removeList(&(village->hosp.assess), p);
					village->hosp.realloc_lock.lock();
					addList(&(village->back->hosp.realloc), p);
					village->hosp.realloc_lock.unlock();
				}
			}
			else /* move to village */
			{
				village->hosp.free_personnel++;
				removeList(&(village->hosp.assess), p);
				addList(&(village->population), p);
			}
		}
	}
}
/**********************************************************************/
void check_patients_waiting(struct Village *village)
{
	struct Patient *list = village->hosp.waiting;
	struct Patient *p;

	while (list != NULL)
	{
		p = list;
		list = list->forward;
		if (village->hosp.free_personnel > 0)
		{
			village->hosp.free_personnel--;
			p->time_left = sim_assess_time;
			p->time += p->time_left;
			removeList(&(village->hosp.waiting), p);
			addList(&(village->hosp.assess), p);
		}
		else
		{
			p->time++;
		}
	}
}
/**********************************************************************/
void check_patients_realloc(struct Village *village)
{
	struct Patient *p, *s;

	while (village->hosp.realloc != NULL)
	{
		// INSIEME FIX
		p = village->hosp.realloc;
		s = p;
		while (p != NULL)
		{
			if (p->id < s->id) s = p;
			p = p->forward;
		}
		removeList(&(village->hosp.realloc), s);
		put_in_hosp(&(village->hosp), s);
	}
}
/**********************************************************************/
void check_patients_population(struct Village *village)
{
	struct Patient *list = village->population;
	struct Patient *p;
	float rand;

	while (list != NULL)
	{
		p = list;
		list = list->forward;
		/* randomize in patient */
		rand = my_rand(&(p->seed));
		if (rand < sim_get_sick_p)
		{
			removeList(&(village->population), p);
			put_in_hosp(&(village->hosp), p);
		}
	}

}
/**********************************************************************/
void put_in_hosp(struct Hosp *hosp, struct Patient *patient)
{
	(patient->hosps_visited)++;

	if (hosp->free_personnel > 0)
	{
		hosp->free_personnel--;
		addList(&(hosp->assess), patient);
		patient->time_left = sim_assess_time;
		patient->time += patient->time_left;
	}
	else
	{
		addList(&(hosp->waiting), patient);
	}
}

template<typename T>
void sim_village_par_internal(struct Village *village, const T& rec_call)
{
	struct Village *vlist;

	std::vector<decltype(allscale::api::core::run(rec_call(nullptr)))> futures;
	/* Traverse village hierarchy (lower level first)*/
	vlist = village->forward;
	while(vlist) {
		//#pragma omp task untied
		futures.push_back(rec_call(vlist));
		vlist = vlist->next;
	}

	/* Uses lists v->hosp->inside, and v->return */
	check_patients_inside(village);

	/* Uses lists v->hosp->assess, v->hosp->inside, v->population and (v->back->hosp->realloc) !!! */
	check_patients_assess_par(village);

	/* Uses lists v->hosp->waiting, and v->hosp->assess */
	check_patients_waiting(village);

	//#pragma omp taskwait
	for(auto& f: futures) {
		f.get();
	}

	/* Uses lists v->hosp->realloc, v->hosp->asses and v->hosp->waiting */
	check_patients_realloc(village);

	/* Uses list v->population, v->hosp->asses and v->h->waiting */
	check_patients_population(village);
}

void sim_village_par(const std::launch l, struct Village *village) {
	allscale::api::core::prec(
		[](struct Village* village) { return !village; },
		[](struct Village* village) { return; },
		[](struct Village* village, const auto& rec_call) {
			sim_village_par_internal(village, rec_call);
		}
	)(village).get();
}

/**********************************************************************/
void my_print(struct Village *village) {
	struct Village *vlist;
	struct Patient *plist;

	if (village == NULL) return;

	/* Traverse village hierarchy (lower level first)*/
	vlist = village->forward;
	while(vlist) {
		my_print(vlist);
		vlist = vlist->next;
	}

	plist = village->population;

	while (plist != NULL) {
		plist = plist->forward;
	}
}

/**********************************************************************/
void read_input_data(const char *filename) {
	FILE *fin = fopen(filename, "r");
	int res;

	if(fin == NULL) {
		std::stringstream ss;
		ss << "Could not open sequence file (" << filename << ")\n";
		inncabs::error(ss.str());
	}
	res = fscanf(fin,"%d %d %d %d %d %d %d %f %f %f %d %d %d %d %d %d %d %d %f",
		&sim_level,
		&sim_cities,
		&sim_population_ratio,
		&sim_time,
		&sim_assess_time,
		&sim_convalescence_time,
		&sim_seed,
		&sim_get_sick_p,
		&sim_convalescence_p,
		&sim_realloc_p,
		&res_population,
		&res_hospitals,
		&res_personnel,
		&res_checkin,
		&res_village,
		&res_waiting,
		&res_assess,
		&res_inside,
		&res_avg_stay
		);
	if(res == EOF) {
		std::stringstream ss;
		ss << "Bogus input file (" << filename << ")\n";
		inncabs::error(ss.str());
	}
	fclose(fin);

	// Printing input data
	std::stringstream ss;
	ss << "\n";
	ss << "Number of levels    = " << (int) sim_level << "\n";
	ss << "Cities per level    = " << (int) sim_cities << "\n";
	ss << "Population ratio    = " << (int) sim_population_ratio << "\n";
	ss << "Simulation time     = " << (int) sim_time << "\n";
	ss << "Assess time         = " << (int) sim_assess_time << "\n";
	ss << "Convalescence time  = " << (int) sim_convalescence_time << "\n";
	ss << "Initial seed        = " << (int) sim_seed << "\n";
	ss << "Get sick prob.      = " << (float) sim_get_sick_p << "\n";
	ss << "Convalescence prob. = " << (float) sim_convalescence_p << "\n";
	ss << "Realloc prob.       = " << (float) sim_realloc_p << "\n";
	inncabs::message(ss.str());
}

bool check_village(struct Village *top) {
	struct Results result = get_results(top);
	bool answer = true;

	if(res_population != result.total_patients) answer = false;
	if(res_hospitals != result.hosps_number) answer = false;
	if(res_personnel != result.hosps_personnel) answer = false;
	if(res_checkin != result.total_hosps_v) answer = false;
	if(res_village != result.total_in_village) answer = false;
	if(res_waiting != result.total_waiting) answer = false;
	if(res_assess != result.total_assess) answer = false;
	if(res_inside != result.total_inside) answer = false;

	std::stringstream ss;
	ss << "\n";
	ss << "Sim. Variables      = expect / result\n";
	ss << "Total population    = " << (int) res_population << " / " << (int) result.total_patients << " people\n";
	ss << "Hospitals           = " << (int) res_hospitals << " / " << (int) result.hosps_number << " people\n";
	ss << "Personnel           = " << (int) res_personnel << " / " << (int) result.hosps_personnel << " people\n";
	ss << "Check-in's          = " << (int) res_checkin << " / " << (int) result.total_hosps_v << " people\n";
	ss << "In Villages         = " << (int) res_village << " / " << (int) result.total_in_village << " people\n";
	ss << "In Waiting List     = " << (int) res_waiting << " / " << (int) result.total_waiting << " people\n";
	ss << "In Assess           = " << (int) res_assess << " / " << (int) result.total_assess << " people\n";
	ss << "Inside Hospital     = " << (int) res_inside << " / " << (int) result.total_inside << " people\n";
	ss << "Average Stay        = " << (float) res_avg_stay << " / " << (float) result.total_time/result.total_patients << "u/time\n";
	inncabs::message(ss.str());

	my_print(top);

	return answer;
}
/**********************************************************************/
void sim_village_main_par(const std::launch l, struct Village *top) {
	long i;
	for(i = 0; i < sim_time; i++) sim_village_par(l, top);
}
