#ifndef HAVE_TYPEDEFS
#  define HAVE_TYPEDEFS

#include <stdint.h>
#define FILENAMESIZE 200

typedef struct Boot Boot;
typedef struct BootChr BootChr;
typedef struct Bounds Bounds;
typedef struct BranchTab BranchTab;
typedef struct Constraint Constraint;
typedef struct El El;
typedef struct Gene Gene;
typedef struct GPTree GPTree;
typedef struct HashTab HashTab;
typedef struct HashTabSeq HashTabSeq;
typedef struct LblNdx LblNdx;
typedef struct NodeStore NodeStore;
typedef enum   ParamStatus ParamStatus;
typedef enum   ParamType ParamType;
typedef struct ParKeyVal ParKeyVal;
typedef struct ParStore ParStore;
typedef struct PopNode PopNode;
typedef struct PopNodeTab PopNodeTab;
typedef struct SimSched SimSched;
typedef struct SampNdx SampNdx;
typedef struct StrInt StrInt;
typedef struct Tokenizer Tokenizer;
typedef struct DAFReader DAFReader;

/// Distinguish between parameters that free, fixed, Gaussian, or
/// constrained.  Free parameters can be changed during optimization;
/// fixed ones never change; Gaussian ones are reset for each
/// simulation replicate by sampling from a truncated normal
/// distribution; Constrained ones are functions of one or more free
/// parameters.
enum ParamStatus {Free, Fixed, Gaussian, Constrained};

/// Distinguish between parameters that describe population size,
/// time, and gene flow.
enum ParamType { TwoN, Time, MixFrac };

#if 1
#  define TIPID_SIZE 32
typedef uint32_t tipId_t;
#else
#  define TIPID_SIZE 64
typedef uint64_t tipId_t;
#endif

#define KL_COST 1
#define CHISQR_COST 2
#define SMPLCHISQR_COST 3
#define POISSON_COST 4
#define LNL_COST 5
#define COST LNL_COST

#endif
