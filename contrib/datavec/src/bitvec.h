#ifndef BITVEC_H
#define BITVEC_H

#include "utils/varbit.h"

VarBit	   *InitBitVector(int dim);

extern "C" {
    Datum hamming_distance(PG_FUNCTION_ARGS);
    Datum jaccard_distance(PG_FUNCTION_ARGS);
}

#endif
