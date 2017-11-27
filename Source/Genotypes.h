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
    uint8_t *gen /* .bed */; // length = (snp_cnt * ind_cnt + 3) / 4 
    //PLINK : snp_cnt * (ind_cnt + 3) / 4
    uint8_t *phn /* .fam */; // length = (ind_cnt + ceil(log2(fen_uni)) - 1) / ceil(log2(fen_uni))
    size_t phn_uni /* .fam */;
    size_t snp_cnt /* + .bim */, ind_cnt /* .fam */, gen_cnt /* = snp_cnt * ind_cnt */;
    size_t *chr_len /* + .bim */; // length = chr_cnt
    size_t chr_cnt /* + .bim */;
    uint32_t *pos /* + .bim */; // length = snp_cnt
    ptrdiff_t *snpname_off /* + .bim */; // length = snp_cnt
    char *snpname /* + .bim */; // length = snpname_sz
    size_t snpname_sz /* + .bim */;
    uint8_t *all /* ? .bim */; // length = (snp_cnt + 3) / 4
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
    char *path_bim, *path_bed, *path_fam;
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
