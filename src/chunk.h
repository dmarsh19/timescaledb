/*
 * This file and its contents are licensed under the Apache License 2.0.
 * Please see the included NOTICE for copyright information and
 * LICENSE-APACHE for a copy of the license.
 */
#ifndef TIMESCALEDB_CHUNK_H
#define TIMESCALEDB_CHUNK_H

#include <postgres.h>
#include <access/htup.h>
#include <access/tupdesc.h>
#include <utils/hsearch.h>
#include <foreign/foreign.h>

#include "export.h"
#include "catalog.h"
#include "chunk_constraint.h"
#include "hypertable.h"
#include "export.h"

typedef struct Hypercube Hypercube;
typedef struct Point Point;
typedef struct Hyperspace Hyperspace;
typedef struct Hypertable Hypertable;

/*
 * A chunk represents a table that stores data, part of a partitioned
 * table.
 *
 * Conceptually, a chunk is a hypercube in an N-dimensional space. The
 * boundaries of the cube is represented by a collection of slices from the N
 * distinct dimensions.
 */
typedef struct Chunk
{
	FormData_chunk fd;
	char relkind;
	Oid table_id;
	Oid hypertable_relid;

	/*
	 * The hypercube defines the chunks position in the N-dimensional space.
	 * Each of the N slices in the cube corresponds to a constraint on the
	 * chunk table.
	 */
	Hypercube *cube;
	ChunkConstraints *constraints;

	/*
	 * The servers that hold a copy of the chunk. NIL for non-distributed
	 * hypertables.
	 */
	List *servers;
} Chunk;

/*
 * ChunkScanCtx is used to scan for chunks in a hypertable's N-dimensional
 * hyperspace.
 *
 * For every matching constraint, a corresponding chunk will be created in the
 * context's hash table, keyed on the chunk ID.
 */
typedef struct ChunkScanCtx
{
	HTAB *htab;
	Hyperspace *space;
	Point *point;
	unsigned int num_complete_chunks;
	bool early_abort;
	LOCKMODE lockmode;
	void *data;
} ChunkScanCtx;

/* Returns true if the chunk has a full set of constraints, otherwise
 * false. Used to find a chunk matching a point in an N-dimensional
 * hyperspace. */
static inline bool
chunk_is_complete(Chunk *chunk, Hyperspace *space)
{
	return space->num_dimensions == chunk->constraints->num_dimension_constraints;
}

/* The hash table entry for the ChunkScanCtx */
typedef struct ChunkScanEntry
{
	int32 chunk_id;
	Chunk *chunk;
} ChunkScanEntry;

extern Chunk *ts_chunk_create_from_point(Hypertable *ht, Point *p, const char *schema,
										 const char *prefix);
extern Chunk *ts_chunk_create_stub(int32 id, int16 num_constraints, const char relkind);
extern Chunk *ts_chunk_find(Hyperspace *hs, Point *p);
extern Chunk **ts_chunk_find_all(Hyperspace *hs, List *dimension_vecs, LOCKMODE lockmode,
								 unsigned int *num_chunks);
extern List *ts_chunk_find_all_oids(Hyperspace *hs, List *dimension_vecs, LOCKMODE lockmode);

extern Chunk *ts_chunk_copy(Chunk *chunk);
extern Chunk *ts_chunk_get_by_name_with_memory_context(const char *schema_name,
													   const char *table_name,
													   int16 num_constraints, MemoryContext mctx,
													   bool fail_if_not_found);
extern TSDLLEXPORT Chunk *ts_chunk_get_by_id(int32 id, int16 num_constraints,
											 bool fail_if_not_found);
extern TSDLLEXPORT Chunk *ts_chunk_get_by_relid(Oid relid, int16 num_constraints,
												bool fail_if_not_found);
extern Oid ts_chunk_get_relid(int32 chunk_id, bool missing_ok);
extern Oid ts_chunk_get_schema_id(int32 chunk_id, bool missing_ok);
extern bool ts_chunk_get_id(const char *schema, const char *table, int32 *chunk_id);
extern Chunk *ts_chunk_get_by_id(int32 id, int16 num_constraints, bool fail_if_not_found);
extern bool ts_chunk_exists(const char *schema_name, const char *table_name);
extern bool ts_chunk_exists_relid(Oid relid);
extern void ts_chunk_recreate_all_constraints_for_dimension(Hyperspace *hs, int32 dimension_id);
extern int ts_chunk_delete_by_relid(Oid chunk_oid);
extern int ts_chunk_delete_by_hypertable_id(int32 hypertable_id);
extern int ts_chunk_delete_by_name(const char *schema, const char *table);
extern bool ts_chunk_set_name(Chunk *chunk, const char *newname);
extern bool ts_chunk_set_schema(Chunk *chunk, const char *newschema);
extern List *ts_chunk_get_window(int32 dimension_id, int64 point, int count, MemoryContext mctx);
extern void ts_chunks_rename_schema_name(char *old_schema, char *new_schema);
extern TSDLLEXPORT List *ts_chunk_do_drop_chunks(Oid table_relid, Datum older_than_datum,
												 Datum newer_than_datum, Oid older_than_type,
												 Oid newer_than_type, bool cascade,
												 bool cascades_to_materializations,
												 int32 log_level);
extern TSDLLEXPORT Chunk *ts_chunk_find_or_create_without_cuts(Hypertable *ht, Hypercube *hc,
															   const char *schema_name,
															   const char *table_name,
															   bool *created);
extern List *ts_chunk_servers_copy(Chunk *chunk);

extern TSDLLEXPORT List *ts_chunk_find_chunk_ids_by_hypertable_id(int32 hypertable_id);

#define chunk_get_by_name(schema_name, table_name, num_constraints, fail_if_not_found)             \
	ts_chunk_get_by_name_with_memory_context(schema_name,                                          \
											 table_name,                                           \
											 num_constraints,                                      \
											 CurrentMemoryContext,                                 \
											 fail_if_not_found)

#endif /* TIMESCALEDB_CHUNK_H */
