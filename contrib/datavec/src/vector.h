#ifndef VECTOR_H
#define VECTOR_H

#define VECTOR_MAX_DIM 16000

#define VECTOR_SIZE(_dim)		(offsetof(Vector, x) + sizeof(float)*(_dim))
#define DatumGetVector(x)		((Vector *) PG_DETOAST_DATUM(x))
#define PG_GETARG_VECTOR_P(x)	DatumGetVector(PG_GETARG_DATUM(x))
#define PG_RETURN_VECTOR_P(x)	PG_RETURN_POINTER(x)
#define UpdateProgress(index, val) ((void)val)

typedef struct Vector
{
	int32		vl_len_;		/* varlena header (do not touch directly!) */
	int16		dim;			/* number of dimensions */
	int16		unused;			/* reserved for future use, always zero */
	float		x[FLEXIBLE_ARRAY_MEMBER];
}			Vector;

Vector	   *InitVector(int dim);
void		PrintVector(char *msg, Vector * vector);
int			vector_cmp_internal(Vector * a, Vector * b);
void log_newpage_range(Relation rel, ForkNumber forknum, BlockNumber startblk, BlockNumber endblk, bool page_std);
int PlanCreateIndexWorkers(Relation heapRelation, IndexInfo *indexInfo);

extern "C" {
    PGDLLEXPORT Datum vector_in(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_out(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_typmod_in(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_recv(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_send(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum array_to_vector(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_to_float4(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum l2_distance(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_l2_squared_distance(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum inner_product(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_negative_inner_product(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum cosine_distance(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_spherical_distance(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_dims(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_norm(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_add(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_sub(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_le(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_lt(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_eq(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_ne(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_ge(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_gt(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_cmp(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_accum(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_combine(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_avg(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum l1_distance(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum l2_normalize(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum binary_quantize(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum subvector(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_mul(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum vector_concat(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum halfvec_l2_normalize(PG_FUNCTION_ARGS);
    PGDLLEXPORT Datum sparsevec_l2_normalize(PG_FUNCTION_ARGS);
}

#endif
