#pragma once

#include "Common.h"
#include "Framework.h"
#include "LoopMT.h"

enum
{
    GENOTYPESCONTEXT_BIT_POS_OPTION0,
    GENOTYPESCONTEXT_BIT_POS_OPTION1,
    GENOTYPESCONTEXT_BIT_POS_OPTION2,
    GENOTYPESCONTEXT_BIT_POS_OPTION3,
    GENOTYPESCONTEXT_BIT_CNT
};

enum
{
    GENOTYPES_STAT_BIT_POS_INIT_COMP = 0,
    GENOTYPES_STAT_BIT_POS_INIT_SUCC,
    GENOTYPES_STAT_BIT_POS_TASK_COMP,
    GENOTYPES_STAT_BIT_POS_TASK_SUCC,
    GENOTYPES_STAT_BIT_CNT
};

typedef struct genotypesOut genotypesOut;
typedef struct genotypesContext genotypesContext;

typedef struct
{
    genotypesOut *out;
    genotypesContext *context;
} genotypesThreadProcContext;

typedef struct {
    uint8_t *gen;
    size_t gencnt;
    // @Lisa: More genotypes-related fields should be added here...
} genotypesRes, *genotypesResPtr;

typedef struct
{
    // Status bits array
    volatile uint8_t stat[BYTE_CNT(GENOTYPES_STAT_BIT_CNT)];
    // Data release count 
    volatile size_t hold;

    // Multithreaded loop object
    loopMT *lmt;
    // Synchronization data for multithreaded loop
    loopMTSync sync;

    // Contexts for thread functions
    genotypesThreadProcContext context;
} genotypesSupp;

struct genotypesContext
{
    char *path;
    uint8_t bits[BYTE_CNT(GENOTYPESCONTEXT_BIT_CNT)];
};

typedef struct {
    frameworkMetadata meta;
    genotypesResPtr res;
    volatile uint8_t *stat;
    volatile size_t *hold; 
} genotypesMetadata;

typedef frameworkMetadata genotypesIn;

#define GENOTYPES_META(IN) ((genotypesMetadata *) (IN))
#define GENOTYPES_META_INIT(IN, OUT, CONTEXT) { .meta = *FRAMEWORK_META(IN), .res = &(OUT)->res, .stat = (OUT)->supp.stat, .hold = &(OUT)->supp.hold }

struct genotypesOut
{
    genotypesMetadata meta;
    genotypesRes res;

    // All synchronization data goes here
    genotypesSupp supp;
};

void genotypesContextDispose(genotypesContext *);
bool genotypesPrologue(genotypesIn *, genotypesOut **, genotypesContext *);
bool genotypesEpilogue(genotypesIn *, genotypesOut *, void *);
