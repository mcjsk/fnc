#!/bin/bash

if [[ x = "x$1" ]]; then
    echo "Usage: $0 struct_name"
    exit 1
fi

s=$1
cat <<EOF
/**

*/
struct $s {
  int dummy;
};

/** Convenience typedef. */
typedef struct $s $s;

/** Initialized-with-defaults $s structure, intended for
    const-copy initialization. */
#define ${s}_empty_m {0}

/** Initialized-with-defaults $s structure, intended for
    non-const copy initialization. */
extern const $s ${s}_empty;

// Put this in a C file:
const $s ${s}_empty = ${s}_empty_m;

EOF
