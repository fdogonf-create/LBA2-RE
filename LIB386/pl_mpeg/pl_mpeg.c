/* The one translation unit that holds pl_mpeg's implementation. The header
   uses size_t and FILE without including their headers, so they come first. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PL_MPEG_IMPLEMENTATION
#include "pl_mpeg.h"
