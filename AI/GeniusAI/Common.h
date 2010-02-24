#ifndef GENIUS_COMMON_H
#define GENIUS_COMMON_H

// GCC-compiler compatibility hack.
#ifdef __GNUC__
#define strcpy_s(a, b, c) strncpy(a, c, b)
#endif

#include "../../AI_Base.h"
#include "../../global.h"

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH;

void debugBox(const char *msg, bool messageBox = false);

#endif // GENIUS_COMMON_H