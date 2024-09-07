-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "ALTER EXTENSION datavec UPDATE TO '0.1.1'" to load this file. \quit

CREATE FUNCTION vector_recv(internal, oid, integer) RETURNS vector
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION vector_send(vector) RETURNS bytea
	AS 'MODULE_PATHNAME' LANGUAGE C IMMUTABLE STRICT;

ALTER TYPE vector SET ( RECEIVE = vector_recv, SEND = vector_send );

-- functions

ALTER FUNCTION vector_in(cstring, oid, integer) ;
ALTER FUNCTION vector_out(vector) ;
ALTER FUNCTION vector_typmod_in(cstring[]) ;
ALTER FUNCTION vector_recv(internal, oid, integer) ;
ALTER FUNCTION vector_send(vector) ;
ALTER FUNCTION l2_distance(vector, vector) ;
ALTER FUNCTION inner_product(vector, vector) ;
ALTER FUNCTION cosine_distance(vector, vector) ;
ALTER FUNCTION vector_dims(vector) ;
ALTER FUNCTION vector_norm(vector) ;
ALTER FUNCTION vector_add(vector, vector) ;
ALTER FUNCTION vector_sub(vector, vector) ;
ALTER FUNCTION vector_lt(vector, vector) ;
ALTER FUNCTION vector_le(vector, vector) ;
ALTER FUNCTION vector_eq(vector, vector) ;
ALTER FUNCTION vector_ne(vector, vector) ;
ALTER FUNCTION vector_ge(vector, vector) ;
ALTER FUNCTION vector_gt(vector, vector) ;
ALTER FUNCTION vector_cmp(vector, vector) ;
ALTER FUNCTION vector_l2_squared_distance(vector, vector) ;
ALTER FUNCTION vector_negative_inner_product(vector, vector) ;
ALTER FUNCTION vector_spherical_distance(vector, vector) ;
ALTER FUNCTION vector(vector, integer, boolean) ;
ALTER FUNCTION array_to_vector(integer[], integer, boolean) ;
ALTER FUNCTION array_to_vector(real[], integer, boolean) ;
ALTER FUNCTION array_to_vector(double precision[], integer, boolean) ;
