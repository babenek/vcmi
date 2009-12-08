#ifndef __GENIUS_COMMON__
#define __GENIUS_COMMON__

#ifdef __GNUC__
#define strcpy_s(a, b, c) strncpy(a, c, b)
#endif

#pragma warning(push, 0)
#include "AI_Base.h"
#pragma warning(pop)

void debugBox(const char *msg, bool messageBox = false);

#endif/*__GENIUS_COMMON__*/