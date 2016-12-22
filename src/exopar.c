#include "exopar.h"
#include "misc.h"
#include "dtnorm.h"
#include <string.h>
#include <assert.h>
#include <gsl/gsl_randist.h>

typedef struct ExoParList ExoParList;
typedef struct ExoParItem ExoParItem;

struct ExoParItem {
    double     *ptr;            // not locally owned
    double      mean, sd;
};

struct ExoParList {
    double     *ptr;            // not locally owned
    ExoParItem  par;
    ExoParList *next;
};

struct ExoPar {
    ExoParList *list;
    int         frozen;
    int         n;
    ExoParItem *v;
};

static void ExoParItem_init(ExoParItem * self, double *ptr, double mean,
                            double sd);
static void ExoParItem_sample(const ExoParItem * self, double low,
                              double high, gsl_rng * rng);
static int  ExoParList_size(ExoParList * self);
static ExoParList *ExoParList_add(ExoParList * old, double *ptr, double m,
                                  double sd);
static void ExoParList_free(ExoParList * self);
static int  compare_ExoParItem_ExoParItem(const void *void_x,
                                          const void *void_y);
static int  compare_dblPtr_ExoParItem(const void *void_x, const void *void_y);

static void ExoParItem_init(ExoParItem * self, double *ptr, double mean,
                            double sd) {
    assert(ptr);
    self->ptr = ptr;
    *self->ptr = self->mean = mean;
    self->sd = sd;
}

static void ExoParItem_sample(const ExoParItem * self, double low,
                              double high, gsl_rng * rng) {
    double      x;
    assert(low < high);

    // Doubly-truncated normal random variate
    x = dtnorm(self->mean, self->sd, low, high, rng);

    *self->ptr = x;
}

/// Add a new link to list.
/// ptr points to the memory occupied by the parameter; m is the mean,
/// sd the standard deviation.
static ExoParList *ExoParList_add(ExoParList * old, double *ptr, double m,
                                  double sd) {
    assert(ptr);
    ExoParList *new = malloc(sizeof(ExoParList));
    CHECKMEM(new);

    ExoParItem_init(&new->par, ptr, m, sd);
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

static int compare_ExoParItem_ExoParItem(const void *void_x,
                                         const void *void_y) {
    const ExoParItem *x = (const ExoParItem *) void_x;
    const ExoParItem *y = (const ExoParItem *) void_y;

    if(x->ptr > y->ptr)
        return 1;
    else if(x->ptr < y->ptr)
        return -1;
    assert(x->ptr == y->ptr);
    return 0;
}

static int compare_dblPtr_ExoParItem(const void *void_x, const void *void_y) {
    double     *const *x = (double *const *) void_x;
    const ExoParItem *y = (const ExoParItem *) void_y;

    if(*x > y->ptr)
        return 1;
    else if(*x < y->ptr)
        return -1;
    assert(*x == y->ptr);
    return 0;
}

/// Create a new ExoPar object by coping ExoParItem values
/// from linked list and then sorting them.
ExoPar     *ExoPar_new(void) {
    ExoPar     *self = malloc(sizeof(ExoPar));
    CHECKMEM(self);

    self->list = NULL;
    self->frozen = 0;
    self->n = 0;
    self->v = NULL;
    return self;
}

void ExoPar_freeze(ExoPar * self) {

    if(self->frozen)
        DIE("Can't freeze an ExoPar twice");
    self->frozen = 1;
    self->n = ExoParList_size(self->list);
    self->v = malloc(self->n * sizeof(ExoParItem));
    CHECKMEM(self->v);
    for(int i = 0; i < self->n; ++i) {
        assert(self->list != NULL);
        memcpy(self->v + i, &self->list->par, sizeof(ExoParItem));
        self->list = self->list->next;
    }
    qsort(self->v, (size_t) self->n, sizeof(ExoParItem),
          compare_ExoParItem_ExoParItem);
    ExoParList_free(self->list);
    self->list = NULL;
}

/// If ptr is in ExoPar, then reset the value it points to with a
/// random value generated by sampling from the normal distribution
/// and then reflecting back and forth so that the value lies within
/// [low, high]. Return 0 on success or 1 if ptr is not in ExoPar.
int ExoPar_sample(ExoPar * self, double *ptr,
                  double low, double high, gsl_rng * rng) {
    assert(self->frozen);
    const ExoParItem *exopar = bsearch(&ptr, self->v,
                                       (size_t) self->n,
                                       sizeof(ExoParItem),
                                       compare_dblPtr_ExoParItem);
    if(exopar) {
        ExoParItem_sample(exopar, low, high, rng);
        return 0;
    }
    return 1;
}

void ExoPar_free(ExoPar * self) {
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
void ExoPar_add(ExoPar * self, double *ptr, double m, double sd) {
    assert(ptr);
    self->list = ExoParList_add(self->list, ptr, m, sd);
}

/// Duplicate an ExoPar object.
ExoPar     *ExoPar_dup(const ExoPar * old) {
    if(!old->frozen)
        DIE("Can't dup an unfrozen ExoPar");

    ExoPar     *new = malloc(sizeof(ExoPar));
    CHECKMEM(new);

    new->n = old->n;
    new->frozen = old->frozen;
    new->list = NULL;
    new->v = memdup(old->v, old->n * sizeof(old->v[0]));
    CHECKMEM(new->v);
    return new;
}

void ExoPar_shiftPtrs(ExoPar * self, size_t offset, int sign) {
    if(!self->frozen)
        DIE("Can't shift pointers in an unfrozen ExoPar");
    int         i;
    for(i = 0; i < self->n; ++i)
        SHIFT_PTR(self->v[i].ptr, offset, sign);
}
