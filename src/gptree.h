#ifndef ARR_GPTREE_H
#  define ARR_GPTREE_H

#  include "typedefs.h"
#  include <stdio.h>
#  include <gsl/gsl_rng.h>

GPTree     *GPTree_new(const char *fname, Bounds bnd);
void        GPTree_free(GPTree *self);
GPTree     *GPTree_dup(const GPTree *old);
void        GPTree_sanityCheck(GPTree *self, const char *file, int line);
int         GPTree_equals(const GPTree *lhs, const GPTree *rhs);
LblNdx      GPTree_getLblNdx(GPTree *self);
void        GPTree_simulate(GPTree *self, BranchTab *branchtab,
                            gsl_rng *rng, unsigned long nreps,
                            int doSing);
int         GPTree_nFree(const GPTree *self);
double     *GPTree_loBounds(GPTree *self);
double     *GPTree_upBounds(GPTree *self);
unsigned    GPTree_nsamples(GPTree *self);
void        GPTree_setParams(GPTree *self, int n, double x[n]);
void        GPTree_getParams(GPTree *self, int n, double x[n]);
void        GPTree_randomize(GPTree *self, gsl_rng *rng);
void        GPTree_printParStore(GPTree *self, FILE *fp);
void        GPTree_printParStoreFree(GPTree *self, FILE *fp);
int         GPTree_feasible(const GPTree *self, int verbose);

#endif
