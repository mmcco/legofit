#include "exopar.h"
#include "misc.h"
#include <string.h>
#include <assert.h>
#include <gsl/gsl_randist.h>

struct ExoPar {
    double     *ptr;            // not locally owned
    double      mean, sd;
};

struct ExoParList {
    double     *ptr;            // not locally owned
    ExoPar      par;
    ExoParList *next;
};

struct ExoParTab {
    ExoParList *list;
    int         frozen;
    int         n;
    ExoPar     *v;
};

static void ExoPar_init(ExoPar * self, double *ptr, double mean, double sd);
static void ExoPar_sample(const ExoPar * self, double low, double high,
                            gsl_rng * rng);
static int  ExoParList_size(ExoParList * self);
static ExoParList *ExoParList_add(ExoParList * old, double *ptr, double m,
                                  double sd);
static void ExoParList_free(ExoParList * self);
static int  compare_ExoPar_ExoPar(const void *void_x, const void *void_y);
static int  compare_dblPtr_ExoPar(const void *void_x, const void *void_y);

static void ExoPar_init(ExoPar * self, double *ptr, double mean, double sd) {
    self->ptr = ptr;
    *self->ptr = self->mean = mean;
    self->sd = sd;
}

static void ExoPar_sample(const ExoPar * self, double low, double high,
                            gsl_rng * rng) {
    double      x;
    fprintf(stderr,"%s: low=%lf high=%lf\n",
            __func__, low, high);
    assert(low < high);
    if(self->sd == 0.0)
        return;
    x = self->mean + gsl_ran_gaussian(rng, self->sd);
    x = reflect(x, low, high);
    *self->ptr = x;
}

/// Add a new link to list.
/// ptr points to the memory occupied by the parameter; m is the mean,
/// sd the standard deviation.
static ExoParList *ExoParList_add(ExoParList * old, double *ptr, double m,
                                  double sd) {
    ExoParList *new = malloc(sizeof(ExoParList));
    CHECKMEM(new);

    ExoPar_init(&new->par, ptr, m, sd);
    new->next = old;
    return new;
}

static void ExoParList_free(ExoParList * self) {
    if(self == NULL)
        return;
    ExoParList_free(self->next);
    free(self);
}

/// Count links in list
static int ExoParList_size(ExoParList * self) {
    int         n = 0;
    while(self != NULL) {
        ++n;
        self = self->next;
    }
    return n;
}

static int compare_ExoPar_ExoPar(const void *void_x, const void *void_y) {
    const ExoPar *x = (const ExoPar *) void_x;
    const ExoPar *y = (const ExoPar *) void_y;

    if(x->ptr > y->ptr)
        return 1;
    else if(x->ptr < y->ptr)
        return -1;
    assert(x->ptr == y->ptr);
    return 0;
}

static int compare_dblPtr_ExoPar(const void *void_x, const void *void_y) {
    double     *const *x = (double *const *) void_x;
    const ExoPar *y = (const ExoPar *) void_y;

    if(*x > y->ptr)
        return 1;
    else if(*x < y->ptr)
        return -1;
    assert(*x == y->ptr);
    return 0;
}

/// Create a new ExoParTab object by coping ExoPar values
/// from linked list and then sorting them.
ExoParTab  *ExoParTab_new(void) {
    ExoParTab  *self = malloc(sizeof(ExoParTab));
    CHECKMEM(self);

    self->list = NULL;
    self->frozen = 0;
    self->n = 0;
    self->v = NULL;
    return self;
}

void ExoParTab_freeze(ExoParTab * self) {

    if(self->frozen)
        DIE("Can't freeze an ExoParTab twice");
    self->frozen = 1;
    self->n = ExoParList_size(self->list);
    self->v = malloc(self->n * sizeof(ExoPar));
    CHECKMEM(self->v);
    for(int i = 0; i < self->n; ++i) {
        assert(self->list != NULL);
        memcpy(self->v + i, &self->list->par, sizeof(ExoPar));
        self->list = self->list->next;
    }
    qsort(self->v, (size_t) self->n, sizeof(ExoPar), compare_ExoPar_ExoPar);
    ExoParList_free(self->list);
    self->list = NULL;
}

/// Return a new value sampled from the distribution associated with
/// ptr.
void ExoParTab_sample(ExoParTab * self, double *ptr,
                        double low, double high, gsl_rng * rng) {
    assert(self->frozen);
    const ExoPar *exopar = bsearch(&ptr, self->v,
                                   (size_t) self->n,
                                   sizeof(ExoPar),
                                   compare_dblPtr_ExoPar);
    if(exopar)
        ExoPar_sample(exopar, low, high, rng);
}

void ExoParTab_free(ExoParTab * self) {
    if(self->frozen) {
        free(self->v);
        assert(self->list == NULL);
    } else {
        ExoParList_free(self->list);
        assert(self->v == NULL);
    }
    free(self);
}

// Add an exogeneous parameter to the table.
// ptr points to the memory occupied by the parameter; m is the mean,
// sd the standard deviation.
void ExoParTab_add(ExoParTab * self, double *ptr, double m, double sd) {
    self->list = ExoParList_add(self->list, ptr, m, sd);
}

/// Duplicate an ExoParTab object.
ExoParTab  *ExoParTab_dup(const ExoParTab *old) {
    if(!old->frozen)
        DIE("Can't dup an unfrozen ExoParTab");

    ExoParTab *new = malloc(sizeof(ExoParTab));
    CHECKMEM(new);

    new->n = old->n;
    new->frozen = old->frozen;
    new->list = NULL;
    new->v = memdup(old->v, old->n * sizeof(old->v[0]));
    CHECKMEM(new->v);
    return new;
}

void        ExoParTab_shiftPtrs(ExoParTab *self, size_t offset) {
    if(!self->frozen)
        DIE("Can't shift pointers in an unfrozen ExoParTab");
    int i;
    for(i=0; i < self->n; ++i) {
        size_t memloc = (size_t) self->v[i].ptr;
        memloc += offset;
        self->v[i].ptr = (double *) memloc;
    }
}
