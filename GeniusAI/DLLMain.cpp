#pragma warning(push, 0)
#include "AI_Base.h"
#pragma warning(pop)

#include "CGeniusAI.h"

// Using namespace not suggested.
using namespace geniusai;

// TODO: Why do we have to make a global variable? Can't we put it simply to
//       GeniusAI, as a private variable?
const char *ai_name = "Genius 1.0";

extern "C" DLL_F_EXPORT int GetGlobalAiVersion()
{
	return AI_INTERFACE_VER;
}

extern "C" DLL_F_EXPORT void GetAiName(char* name)
{
	strcpy_s(name, strlen(ai_name) + 1, ai_name);
}

extern "C" DLL_F_EXPORT char* GetAiNameS()
{
	// need to be defined
	return NULL;
}

extern "C" DLL_F_EXPORT CGlobalAI* GetNewAI()
{
	return new CGeniusAI();
}

extern "C" DLL_F_EXPORT void ReleaseAI(CGlobalAI* i)
{
	delete (CGeniusAI*)i;
}