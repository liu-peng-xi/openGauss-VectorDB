#include "postgres.h"

#include <float.h>

#include "access/amapi.h"
#include "access/reloptions.h"
#include "commands/vacuum.h"
#include "ivfflat.h"
#include "utils/guc.h"
#include "utils/selfuncs.h"
#include "utils/spccache.h"

#if PG_VERSION_NUM < 150000
#define MarkGUCPrefixReserved(x) EmitWarningsOnPlaceholders(x)
#endif

int			ivfflat_probes;
static relopt_kind ivfflat_relopt_kind;

/*
 * Initialize index options and variables
 */
void
IvfflatInit(void)
{
	ivfflat_relopt_kind = add_reloption_kind();
	add_int_reloption(ivfflat_relopt_kind, "lists", "Number of inverted lists",
					  IVFFLAT_DEFAULT_LISTS, IVFFLAT_MIN_LISTS, IVFFLAT_MAX_LISTS
#if PG_VERSION_NUM >= 130000
					  ,AccessExclusiveLock
#endif
		);

	DefineCustomIntVariable("ivfflat.probes", "Sets the number of probes",
							"Valid range is 1..lists.", &ivfflat_probes,
							IVFFLAT_DEFAULT_PROBES, IVFFLAT_MIN_LISTS, IVFFLAT_MAX_LISTS, PGC_USERSET, 0, NULL, NULL, NULL);

	MarkGUCPrefixReserved("ivfflat");
}

/*
 * Estimate the cost of an index scan
 */
static void
ivfflatcostestimate_internal(PlannerInfo *root, IndexPath *path, double loop_count,
					Cost *indexStartupCost, Cost *indexTotalCost,
					Selectivity *indexSelectivity, double *indexCorrelation)
{
	GenericCosts costs;
	int			lists;
	double		ratio;
	double		spc_seq_page_cost;
	Relation	index;

	/* Never use index without order */
	if (path->indexorderbys == NULL)
	{
		*indexStartupCost = DBL_MAX;
		*indexTotalCost = DBL_MAX;
		*indexSelectivity = 0;
		*indexCorrelation = 0;
		return;
	}

	MemSet(&costs, 0, sizeof(costs));

	index = index_open(path->indexinfo->indexoid, NoLock);
	IvfflatGetMetaPageInfo(index, &lists, NULL);
	index_close(index, NoLock);

	/* Get the ratio of lists that we need to visit */
	ratio = ((double) ivfflat_probes) / lists;
	if (ratio > 1.0)
		ratio = 1.0;

	/*
	 * This gives us the subset of tuples to visit. This value is passed into
	 * the generic cost estimator to determine the number of pages to visit
	 * during the index scan.
	 */
	costs.numIndexTuples = path->indexinfo->tuples * ratio;

	genericcostestimate(root, path, loop_count, costs.numIndexTuples, &costs.indexStartupCost,
			&costs.indexTotalCost, &costs.indexSelectivity, &costs.indexCorrelation);

	get_tablespace_page_costs(path->indexinfo->reltablespace, NULL, &spc_seq_page_cost);

	/* Adjust cost if needed since TOAST not included in seq scan cost */
	if (costs.numIndexPages > path->indexinfo->rel->pages && ratio < 0.5)
	{
		/* Change all page cost from random to sequential */
		costs.indexTotalCost -= costs.numIndexPages * (costs.spc_random_page_cost - spc_seq_page_cost);

		/* Remove cost of extra pages */
		costs.indexTotalCost -= (costs.numIndexPages - path->indexinfo->rel->pages) * spc_seq_page_cost;
	}
	else
	{
		/* Change some page cost from random to sequential */
		costs.indexTotalCost -= 0.5 * costs.numIndexPages * (costs.spc_random_page_cost - spc_seq_page_cost);
	}

	/*
	 * If the list selectivity is lower than what is returned from the generic
	 * cost estimator, use that.
	 */
	if (ratio < costs.indexSelectivity)
		costs.indexSelectivity = ratio;

	/* Use total cost since most work happens before first tuple is returned */
	*indexStartupCost = costs.indexTotalCost;
	*indexTotalCost = costs.indexTotalCost;
	*indexSelectivity = costs.indexSelectivity;
	*indexCorrelation = costs.indexCorrelation;
}

/*
 * Parse and validate the reloptions
 */
static bytea *
ivfflatoptions_internal(Datum reloptions, bool validate)
{
	static const relopt_parse_elt tab[] = {
		{"lists", RELOPT_TYPE_INT, offsetof(IvfflatOptions, lists)},
		{"parallel_workers", RELOPT_TYPE_INT, offsetof(StdRdOptions, parallel_workers)}
	};

#if PG_VERSION_NUM >= 130000
	return (bytea *) build_reloptions(reloptions, validate,
									  ivfflat_relopt_kind,
									  sizeof(IvfflatOptions),
									  tab, lengthof(tab));
#else
	relopt_value *options;
	int			numoptions;
	IvfflatOptions *rdopts;

	options = parseRelOptions(reloptions, validate, ivfflat_relopt_kind, &numoptions);
	rdopts = (IvfflatOptions *)allocateReloptStruct(sizeof(IvfflatOptions), options, numoptions);
	fillRelOptions((void *) rdopts, sizeof(IvfflatOptions), options, numoptions,
				   validate, tab, lengthof(tab));

	return (bytea *) rdopts;
#endif
}

/*
 * Validate catalog entries for the specified operator class
 */
static bool
ivfflatvalidate_internal(Oid opclassoid)
{
	return true;
}

/*
 * Define index handler
 *
 * See https://www.postgresql.org/docs/current/index-api.html
 */
PGDLLEXPORT PG_FUNCTION_INFO_V1(ivfflathandler);
Datum
ivfflathandler(PG_FUNCTION_ARGS)
{
	IndexAmRoutine *amroutine = makeNode(IndexAmRoutine);

	amroutine->amstrategies = 0;
	amroutine->amsupport = 5;
#if PG_VERSION_NUM >= 130000
	amroutine->amoptsprocnum = 0;
#endif
	amroutine->amcanorder = false;
	amroutine->amcanorderbyop = true;
	amroutine->amcanbackward = false;	/* can change direction mid-scan */
	amroutine->amcanunique = false;
	amroutine->amcanmulticol = false;
	amroutine->amoptionalkey = true;
	amroutine->amsearcharray = false;
	amroutine->amsearchnulls = false;
	amroutine->amstorage = false;
	amroutine->amclusterable = false;
	amroutine->ampredlocks = false;
	amroutine->amcanparallel = false;
	amroutine->amcaninclude = false;
#if PG_VERSION_NUM >= 130000
	amroutine->amusemaintenanceworkmem = false; /* not used during VACUUM */
	amroutine->amparallelvacuumoptions = VACUUM_OPTION_PARALLEL_BULKDEL;
#endif
	amroutine->amkeytype = InvalidOid;

	/* Interface functions */
	errno_t rc;
	rc = strcpy_s(amroutine->ambuildfuncname, NAMEDATALEN, "ivfflatbuild");
	securec_check(rc, "\0", "\0");
	rc = strcpy_s(amroutine->ambuildemptyfuncname, NAMEDATALEN, "ivfflatbuildempty");
	securec_check(rc, "\0", "\0");
	rc = strcpy_s(amroutine->aminsertfuncname, NAMEDATALEN, "ivfflatinsert");
	securec_check(rc, "\0", "\0");
	rc = strcpy_s(amroutine->ambulkdeletefuncname, NAMEDATALEN, "ivfflatbulkdelete");
	securec_check(rc, "\0", "\0");
	rc = strcpy_s(amroutine->amvacuumcleanupfuncname, NAMEDATALEN, "ivfflatvacuumcleanup");
	securec_check(rc, "\0", "\0");
	rc = strcpy_s(amroutine->amcostestimatefuncname, NAMEDATALEN, "ivfflatcostestimate");
	securec_check(rc, "\0", "\0");
	rc = strcpy_s(amroutine->amoptionsfuncname, NAMEDATALEN, "ivfflatoptions");
	securec_check(rc, "\0", "\0");
	rc = strcpy_s(amroutine->amvalidatefuncname, NAMEDATALEN, "ivfflatvalidate");
	securec_check(rc, "\0", "\0");
	rc = strcpy_s(amroutine->ambeginscanfuncname, NAMEDATALEN, "ivfflatbeginscan");
	securec_check(rc, "\0", "\0");
	rc = strcpy_s(amroutine->amrescanfuncname, NAMEDATALEN, "ivfflatrescan");
	securec_check(rc, "\0", "\0");
	rc = strcpy_s(amroutine->amgettuplefuncname, NAMEDATALEN, "ivfflatgettuple");
	securec_check(rc, "\0", "\0");
	rc = strcpy_s(amroutine->amendscanfuncname, NAMEDATALEN, "ivfflatendscan");
	securec_check(rc, "\0", "\0");

	PG_RETURN_POINTER(amroutine);
}

PGDLLEXPORT PG_FUNCTION_INFO_V1(ivfflatbuild);
Datum
ivfflatbuild(PG_FUNCTION_ARGS)
{
	Relation heap = (Relation)PG_GETARG_POINTER(0);
	Relation index = (Relation)PG_GETARG_POINTER(1);
	IndexInfo *indexinfo = (IndexInfo *)PG_GETARG_POINTER(2);
	IndexBuildResult *result = ivfflatbuild_internal(heap, index, indexinfo);

	PG_RETURN_POINTER(result);
}

PGDLLEXPORT PG_FUNCTION_INFO_V1(ivfflatbuildempty);
Datum
ivfflatbuildempty(PG_FUNCTION_ARGS)
{
	Relation index = (Relation)PG_GETARG_POINTER(0);
	ivfflatbuildempty_internal(index);

	PG_RETURN_VOID();
}

PGDLLEXPORT PG_FUNCTION_INFO_V1(ivfflatinsert);
Datum
ivfflatinsert(PG_FUNCTION_ARGS)
{
	Relation rel = (Relation)PG_GETARG_POINTER(0);
	Datum * values = (Datum *)PG_GETARG_POINTER(1);
	bool *isnull = (bool *)PG_GETARG_POINTER(2);
	ItemPointer ht_ctid = (ItemPointer)PG_GETARG_POINTER(3);
	Relation heaprel = (Relation)PG_GETARG_POINTER(4);
	IndexUniqueCheck checkunique = (IndexUniqueCheck)PG_GETARG_INT32(5);
	bool result = ivfflatinsert_internal(rel, values, isnull, ht_ctid, heaprel, checkunique);

	PG_RETURN_BOOL(result);
}

PGDLLEXPORT PG_FUNCTION_INFO_V1(ivfflatbulkdelete);
Datum
ivfflatbulkdelete(PG_FUNCTION_ARGS)
{
	IndexVacuumInfo *info = (IndexVacuumInfo *)PG_GETARG_POINTER(0);
	IndexBulkDeleteResult *volatile stats = (IndexBulkDeleteResult *)PG_GETARG_POINTER(1);
	IndexBulkDeleteCallback callback = (IndexBulkDeleteCallback)PG_GETARG_POINTER(2);
	void *callback_state = (void *)PG_GETARG_POINTER(3);
	stats = ivfflatbulkdelete_internal(info, stats, callback, callback_state);

	PG_RETURN_POINTER(stats);
}

PGDLLEXPORT PG_FUNCTION_INFO_V1(ivfflatvacuumcleanup);
Datum
ivfflatvacuumcleanup(PG_FUNCTION_ARGS)
{
	IndexVacuumInfo *info = (IndexVacuumInfo *)PG_GETARG_POINTER(0);
	IndexBulkDeleteResult *stats = (IndexBulkDeleteResult *)PG_GETARG_POINTER(1);
	stats = ivfflatvacuumcleanup_internal(info, stats);

	PG_RETURN_POINTER(stats);
}

PGDLLEXPORT PG_FUNCTION_INFO_V1(ivfflatcostestimate);
Datum
ivfflatcostestimate(PG_FUNCTION_ARGS)
{
	PlannerInfo *root = (PlannerInfo *)PG_GETARG_POINTER(0);
	IndexPath *path = (IndexPath *)PG_GETARG_POINTER(1);
	double loopcount = (double)PG_GETARG_FLOAT8(2);
	Cost *startupcost = (Cost *)PG_GETARG_POINTER(3);
	Cost *totalcost = (Cost *)PG_GETARG_POINTER(4);
	Selectivity *selectivity = (Selectivity *)PG_GETARG_POINTER(5);
	double *correlation = (double *)PG_GETARG_POINTER(6);
	ivfflatcostestimate_internal(root, path, loopcount, startupcost, totalcost, selectivity, correlation);

	PG_RETURN_VOID();	
}

PGDLLEXPORT PG_FUNCTION_INFO_V1(ivfflatoptions);
Datum
ivfflatoptions(PG_FUNCTION_ARGS)
{
	Datum reloptions = PG_GETARG_DATUM(0);
	bool validate = PG_GETARG_BOOL(1);
	bytea *result = ivfflatoptions_internal(reloptions, validate);

	if (NULL != result)
		PG_RETURN_BYTEA_P(result);

	PG_RETURN_NULL();
}

PGDLLEXPORT PG_FUNCTION_INFO_V1(ivfflatvalidate);
Datum
ivfflatvalidate(PG_FUNCTION_ARGS)
{
	Oid opclassoid = PG_GETARG_OID(0);
	bool result = ivfflatvalidate_internal(opclassoid);

	PG_RETURN_BOOL(result);
}

PGDLLEXPORT PG_FUNCTION_INFO_V1(ivfflatbeginscan);
Datum
ivfflatbeginscan(PG_FUNCTION_ARGS)
{
	Relation rel = (Relation)PG_GETARG_POINTER(0);
	int nkeys = PG_GETARG_INT32(1);
	int norderbys = PG_GETARG_INT32(2);
	IndexScanDesc scan = ivfflatbeginscan_internal(rel, nkeys, norderbys);

	PG_RETURN_POINTER(scan);
}

PGDLLEXPORT PG_FUNCTION_INFO_V1(ivfflatrescan);
Datum
ivfflatrescan(PG_FUNCTION_ARGS)
{
	IndexScanDesc scan = (IndexScanDesc)PG_GETARG_POINTER(0);
	ScanKey scankey = (ScanKey)PG_GETARG_POINTER(1);
	int nkeys = PG_GETARG_INT32(2);
	ScanKey orderbys = (ScanKey)PG_GETARG_POINTER(3);
	int norderbys = PG_GETARG_INT32(4);
	ivfflatrescan_internal(scan, scankey, nkeys, orderbys, norderbys);

	PG_RETURN_VOID();
}

PGDLLEXPORT PG_FUNCTION_INFO_V1(ivfflatgettuple);
Datum
ivfflatgettuple(PG_FUNCTION_ARGS)
{
	IndexScanDesc scan = (IndexScanDesc)PG_GETARG_POINTER(0);
	ScanDirection direction = (ScanDirection)PG_GETARG_INT32(1);

	if (NULL == scan)
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE), errmsg("Invalid arguments for function ivfflatgettuple")));
	
	bool result = ivfflatgettuple_internal(scan, direction);

	PG_RETURN_BOOL(result);
}

PGDLLEXPORT PG_FUNCTION_INFO_V1(ivfflatendscan);
Datum
ivfflatendscan(PG_FUNCTION_ARGS)
{
	IndexScanDesc scan = (IndexScanDesc)PG_GETARG_POINTER(0);
	ivfflatendscan_internal(scan);

	PG_RETURN_VOID();
}
