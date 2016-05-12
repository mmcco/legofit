/**
 * @file patprob.c
 * @brief Run simulations to estimate site pattern probabilities.
 */

#include "patprob.h"
#include "misc.h"
#include "branchtab.h"
#include "hashtab.h"
#include "parse.h"
#include "parstore.h"
#include "jobqueue.h"
#include "lblndx.h"
#include "gptree.h"
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

typedef struct TaskArg TaskArg;

/** Data structure used by each thread */
struct TaskArg {
    unsigned    rng_seed;
    unsigned long nreps;
    GPTree     *gptree;

    // Returned value
    BranchTab  *branchtab;
};

TaskArg    *TaskArg_new(unsigned rng_seed, GPTree *gptree, unsigned nreps);
void        TaskArg_free(TaskArg * targ);
int         taskfun(void *varg);

/**
 * Construct a new TaskArg by copying a template, but then assign
 * a distinct random number seed.
 */
TaskArg    *TaskArg_new(unsigned rng_seed, GPTree *gptree, unsigned nreps) {
    TaskArg    *a = malloc(sizeof(TaskArg));
    checkmem(a, __FILE__, __LINE__);

    a->rng_seed = rng_seed;
    a->nreps = nreps;
    a->gptree = GPTree_dup(gptree);
    a->branchtab = BranchTab_new();

    return a;
}

/** TaskArg destructor */
void TaskArg_free(TaskArg * self) {
    BranchTab_free(self->branchtab);
    GPTree_free(self->gptree);
    free(self);
}

/** function run by each thread */
int taskfun(void *varg) {
    TaskArg    *targ = (TaskArg *) varg;
    gsl_rng    *rng = gsl_rng_alloc(gsl_rng_taus);
    gsl_rng_set(rng, targ->rng_seed);
    GPTree_simulate(targ->gptree, targ->branchtab, rng, targ->nreps);
    gsl_rng_free(rng);

    return 0;
}

/// Run simulations to estimate site pattern probabilities.
/// On return, pat[i] identifies the i'th pattern, and prob[i]
/// estimates its probability.  Function returns the number of
/// patterns detected--the length of pat and prob.
unsigned patprob(unsigned maxpat,
                 tipId_t pat[maxpat],
                 double prob[maxpat],
                 GPTree *gptree,
                 LblNdx *lblndx,
                 int nTasks,
                 long reps[nTasks],
                 int pointNdx) {
    int j;
    unsigned long currtime = (unsigned long ) time(NULL);

    TaskArg    *taskarg[nTasks];
    unsigned    pid = (unsigned) getpid();

    /*
     * Generate a seed that is unique across points and threads.
     * First step creates a character string that concatenates
     * the decimal representation of time, process id, and index
     * of current point. This string is then hashed to a 32-bit
     * unsigned integer, which stored in a 64-bit unsigned
     * integer, lseed. Inside the loop, we add j to this seed to get
     * the seed for the j'th thread, and apply modulus to avoid
     * overflow.
     */
    long unsigned lseed;
    {
        char s[50]; // strlen(s) should not exceed 40
        snprintf(s, sizeof(s), "%lu%u%u", currtime, pid, pointNdx);
        assert(1+strlen(s) < sizeof(s));
        lseed = strhash(s);
    }
    for(j = 0; j < nTasks; ++j) {
        unsigned seed = (unsigned) ((lseed + j) % UINT_MAX);
        taskarg[j] = TaskArg_new(seed, gptree, reps[j]);
    }

    {
        JobQueue   *jq = JobQueue_new(nTasks);
        if(jq == NULL)
            eprintf("s:%s:%d: Bad return from JobQueue_new",
                    __FILE__, __func__, __LINE__);
        for(j = 0; j < nTasks; ++j)
            JobQueue_addJob(jq, taskfun, taskarg[j]);
        JobQueue_waitOnJobs(jq);
        JobQueue_free(jq);
    }

    // Add all branchtabs into branchtab[0]
    for(j = 1; j < nTasks; ++j)
        BranchTab_plusEquals(taskarg[0]->branchtab, taskarg[j]->branchtab);

    // Put site patterns and branch lengths into arrays.
    unsigned npat = BranchTab_size(taskarg[0]->branchtab);
    assert(npat <= maxpat);
    BranchTab_toArrays(taskarg[0]->branchtab, npat, pat, prob);

    {
        // Normalize so prob distribution sums to 1.
        double      sum = 0.0;
        for(j = 0; j < npat; ++j)
            sum += prob[j];
        for(j = 0; j < npat; ++j)
            prob[j] /= sum;
    }

    *lblndx = taskarg[0]->lblndx;

    for(j = 0; j < nTasks; ++j)
        TaskArg_free(taskarg[j]);
        
    return npat;
}
