#ifndef ARR_POPNODE_H
#  define ARR_POPNODE_H

#  include "typedefs.h"

#  define POPNAMESIZE 30
#  define MAXSAMP ((int)(8*sizeof(tipId_t)))

struct SampNdx {
    unsigned    n;              // number of samples
    char        lbl[MAXSAMP][POPNAMESIZE];
    PopNode    *node[MAXSAMP];
};

struct PopNode {
    int         nparents, nchildren, nsamples;
    double      *twoN;           // ptr to current pop size
    double      *start, *end;    // duration of this PopNode
    double      *mix;            // ptr to frac of pop derived from parent[1]
    struct PopNode *parent[2];
    struct PopNode *child[2];
    Gene       *sample[MAXSAMP];
};

PopNode    *PopNode_new(double *twoN, double *start, NodeStore *ns);
void        PopNode_addChild(PopNode * parent, PopNode * child);
void        PopNode_mix(PopNode * child, double *mPtr, PopNode * introgressor,
                        PopNode * native);
void        PopNode_newGene(PopNode * self, unsigned ndx);
void        PopNode_addSample(PopNode * self, Gene * gene);
Gene       *PopNode_coalesce(PopNode * self, gsl_rng * rng);
void        PopNode_free(PopNode * self);
void        PopNode_clear(PopNode * self);
void        PopNode_print(FILE * fp, PopNode * self, int indent);
void        PopNode_printShallow(PopNode * self, FILE * fp);
PopNode    *PopNode_root(PopNode * self);
void        PopNode_sanityFromLeaf(PopNode * self, const char *file,
                                   int line);
int         PopNode_nsamples(PopNode * self);

void        SampNdx_init(SampNdx * self);
void        SampNdx_addSamples(SampNdx * self, unsigned nsamples,
							   PopNode * pnode);
void        SampNdx_populateTree(SampNdx * self);
unsigned    SampNdx_size(SampNdx * self);

#endif