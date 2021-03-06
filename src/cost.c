/**
   @file cost.c
   @page cost
   @brief Calculate cost function

   @copyright Copyright (c) 2016, Alan R. Rogers
   <rogers@anthro.utah.edu>. This file is released under the Internet
   Systems Consortium License, which can be found in file "LICENSE".
*/

#undef DEBUG
#undef DPRINTF_ON

#include "dprintf.h"
#ifdef DPRINTF_ON
#include <pthread.h>
extern pthread_mutex_t outputLock;
#endif

#include "cost.h"
#include "simsched.h"
#include "gptree.h"
#include "branchtab.h"
#include "patprob.h"
#include "misc.h"
#include <math.h>
#include <gsl/gsl_rng.h>

/// Calculate cost.
/// @param[in] dim dimension of x
/// @param[in] x vector of parameter values.
/// @param jdata void pointer to a CostPar object, which contains
/// exogeneous parameters of the cost function.
/// @param tdata void pointer to a random number generator
/// @return cost
double costFun(int dim, double x[dim], void *jdata, void *tdata) {
    CostPar *cp = (CostPar *) jdata;
    gsl_rng *rng = (gsl_rng *) tdata;

    long nreps = SimSched_getSimReps(cp->simSched);
    DPRINTF(("%s:%d: nreps=%ld\n",__FILE__,__LINE__,nreps));

	GPTree_setParams(cp->gptree, dim, x);
	if(!GPTree_feasible(cp->gptree, 0))
		return HUGE_VAL;

    BranchTab  *prob = patprob(cp->gptree, nreps, cp->doSing, rng);
    BranchTab_divideBy(prob, nreps);
#if COST==KL_COST
    BranchTab_normalize(prob);
    double cost = BranchTab_KLdiverg(cp->obs, prob);
#elif COST==LNL_COST
    BranchTab_normalize(prob);
    double cost = BranchTab_negLnL(cp->obs, prob);
#elif COST==CHISQR_COST
    double cost = BranchTab_chiSqCost(cp->obs, prob, cp->u, cp->nnuc,
                                      nreps);
#elif COST==SMPLCHISQR_COST
    double cost = BranchTab_smplChiSqCost(cp->obs, prob, cp->u, cp->nnuc,
                                      nreps);
#elif COST==POISSON_COST
    double cost = BranchTab_poissonCost(cp->obs, prob, cp->u, cp->nnuc,
                                        nreps);
#else
# error "Unknown cost method"
#endif

    BranchTab_free(prob);

    return cost;
}

/// Duplicate an object of class CostPar.
void * CostPar_dup(const void * arg) {
    assert(arg);
    const CostPar *old = (const CostPar *) arg;
    if(arg==NULL)
        return NULL;
    CostPar *new = memdup(old, sizeof(CostPar));
    CHECKMEM(new);
    new->obs = BranchTab_dup(old->obs);
    CHECKMEM(new->obs);
    new->gptree = GPTree_dup(old->gptree);
    CHECKMEM(new->gptree);
    new->simSched = old->simSched;
    CHECKMEM(new->simSched);
    return new;
}

/// CostPar destructor.
void CostPar_free(void *arg) {
    CostPar *self = (CostPar *) arg;
    if(self)
        free(self);
}
