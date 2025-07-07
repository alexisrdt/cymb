#ifndef CYMB_CYMB_H
#define CYMB_CYMB_H

#include <stdio.h>

#include "cymb/diagnostic.h"
#include "cymb/lex.h"
#include "cymb/memory.h"
#include "cymb/options.h"
#include "cymb/reader.h"
#include "cymb/result.h"
#include "cymb/tree.h"
#include "cymb/version.h"

CymbResult cymbMain(const CymbConstString* arguments, size_t argumentCount);

#endif
