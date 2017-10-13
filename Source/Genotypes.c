#include "Common.h"
#include "Genotypes.h"
#include "TaskMacros.h"

static void genotypesResClose(genotypesRes *res)
{
    free(res->gen);
}

static bool genotypesPatrialProc(loopMTArg *args, genotypesThreadProcContext *context)
{
    FILE *f = fopen(context->context->path, "rb");
    if (!f) goto ERR();

    if (!fseek64(f, args->offset, SEEK_SET) && fread(context->out->res.gen + args->offset, 1, args->length, f) == args->length)
        logMsg(FRAMEWORK_META(context->out)->log, "INFO (%s): Thread %zu have read %zu bytes of the file %s + %zu B.\n", __FUNCTION__, threadPoolFetchThredId(FRAMEWORK_META(context->out)->pool), args->length, context->context->path, args->offset);
    else
        logMsg(FRAMEWORK_META(context->out)->log, "ERROR (%s): Thread %zu cannot read the file %s + %zu B!\n", __FUNCTION__, threadPoolFetchThredId(FRAMEWORK_META(context->out)->pool), context->context->path, args->offset);

    fclose(f);
    return 1;
    
ERR() :
    logError(FRAMEWORK_META(context->out)->log, __FUNCTION__, errno);
    return 0;
}

static bool genotypesThreadPrologue(genotypesOut *args, genotypesContext *context)
{
    const char *strings[] =
    {
        __FUNCTION__,
        "ERROR (%s): Cannot open specified file \"%s\". %s!\n",
        "ERROR (%s): Unable to setup parallel loop!\n"
    };

    enum { STR_FN, STR_FR_EI, STR_FR_LP };
    
    char tempbuff[TEMP_BUFF] = { '\0' };
    FILE *f = NULL;
        
    f = fopen(context->path, "rb");
    if (!f) goto ERR(File);
    
    fseek64(f, 0, SEEK_END);
    size_t sz = ftell64(f);
    
    args->supp.sync = (loopMTSync) {
        .asucc = (aggregatorCallback) bitSet2InterlockedMem,
        .afail = (aggregatorCallback) bitSetInterlockedMem,
        .asuccmem = args->supp.stat, 
        .afailmem = args->supp.stat,
        .asuccarg = pnumGet(FRAMEWORK_META(args)->pnum, GENOTYPES_STAT_BIT_POS_TASK_COMP),
        .afailarg = pnumGet(FRAMEWORK_META(args)->pnum, GENOTYPES_STAT_BIT_POS_TASK_COMP),
    };

    args->supp.context = (genotypesThreadProcContext) { .out = args, .context = context };
    
    args->res.gen = malloc(sz);
    if (sz && !args->res.gen) goto ERR();

    args->supp.lmt = loopMTCreate((loopMTCallback) genotypesPatrialProc, 0, sz, &args->supp.context, FRAMEWORK_META(args)->pool, &args->supp.sync);
    if (!args->supp.lmt) goto ERR(Loop);

    fclose(f);

    for (;;)
    {
        return 1;
    
    ERR():
        logError(FRAMEWORK_META(args)->log, __FUNCTION__, errno);
        break;

    ERR(File):
        strerror_s(tempbuff, sizeof tempbuff, errno);
        logMsg(FRAMEWORK_META(args)->log, strings[STR_FR_EI], strings[STR_FN], context->path, tempbuff);
        break;

    ERR(Loop):
        logMsg(FRAMEWORK_META(args)->log, strings[STR_FR_LP], strings[STR_FN]);
        break;
    }

    if (f) fclose(f);
    return 0;    
}

void genotypesContextDispose(genotypesContext *context)
{
    if (!context) return;

    free(context->path);
    free(context);
}

bool genotypesPrologue(genotypesIn *in, genotypesOut **pout, genotypesContext *context)
{
    genotypesOut *out = *pout = malloc(sizeof *out);
    if (!out) goto ERR();
        
    *out = (genotypesOut) { .meta = GENOTYPES_META_INIT(in, out, context) };
    if (!outInfoSetNext(FRAMEWORK_META(in)->out, out)) goto ERR();

    task *tsk = tasksInfoNextTask(FRAMEWORK_META(in)->tasks);
    if (!tsk) goto ERR();

    if (!pnumTest(FRAMEWORK_META(out)->pnum, GENOTYPES_STAT_BIT_CNT)) goto ERR();
    *tsk = TASK_BIT_1_INIT(genotypesThreadPrologue, NULL, out, context, NULL, out->supp.stat, pnumGet(FRAMEWORK_META(out)->pnum, GENOTYPES_STAT_BIT_POS_INIT_COMP));
    return 1;

ERR() :
    logError(FRAMEWORK_META(in)->log, __FUNCTION__, errno);
    return 0;
}

static bool genotypesThreadClose(genotypesOut *args, void *context)
{
    (void) context;
    if (!args) return 1;

    genotypesResClose(&args->res);
    loopMTDispose(args->supp.lmt);
    return 1;
}

static bool genotypesThreadCloseCondition(genotypesSupp *supp, void *arg)
{
    (void) arg;

    switch (bitGet2((void *) supp->stat, GENOTYPES_STAT_BIT_POS_INIT_COMP))
    {
    case 0: return 0;
    case 1: return 1;
    case 3: break;
    }

    switch (bitGet2((void *) supp->stat, GENOTYPES_STAT_BIT_POS_TASK_COMP))
    {
    case 0: return 0;
    case 1: return 1;
    case 3: break;
    }

    return !(supp->hold);
}

bool genotypesEpilogue(genotypesIn *in, genotypesOut* out, void *context)
{
    if (!out) return 0;
    (void) context;

    task *tsk = tasksInfoNextTask(FRAMEWORK_META(in)->tasks);
    if (!tsk) goto ERR();

    *tsk = (task)
    {
        .arg = out,
        .callback = (taskCallback) genotypesThreadClose,
        .cond = (conditionCallback) genotypesThreadCloseCondition,
        .condmem = &out->supp
    };

    return 1;

ERR():
    logError(FRAMEWORK_META(in)->log, __FUNCTION__, errno);
    return 0;
}