-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "ALTER EXTENSION datavec UPDATE TO '0.6.0'" to load this file. \quit

-- remove this single line for Postgres < 13
ALTER TYPE datavec SET (STORAGE = external);
