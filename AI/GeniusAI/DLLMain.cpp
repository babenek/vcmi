// ****************************************************************************
// DLLMain.cpp
// ----------------------------------------------------------------------------
// Functions that are to be exposed in the GeniusAI.dll library.
// ****************************************************************************
#include "StdAfx.h"
#include "Common.h"
#include "../../AI_Base.h"
#include "CGeniusAI.h"

// TODO: Remake it, so such global-namespace values are better ordered.
namespace geniusai {
  const char *AI_NAME = "Genius 1.0";
}

// Returns the version of GlobalAI.
extern "C" DLL_F_EXPORT si16 GetGlobalAiVersion()
{
	return AI_INTERFACE_VER;
}

// Copies the C-string with the version of AI into the argument.
extern "C" DLL_F_EXPORT void GetAiName(char* name)
{
	strcpy_s(name, strlen(geniusai::AI_NAME) + 1, geniusai::AI_NAME);
}

// Returns a string with the version of AI.
extern "C" DLL_F_EXPORT std::string GetAiNameString(void)
{
	return std::string(geniusai::AI_NAME);
}

// Creates an instance of GeniusAI.
extern "C" DLL_F_EXPORT ::CGlobalAI* GetNewAI()
{
	return new geniusai::CGeniusAI();
}

// Destroys an instance of GeniusAI.
extern "C" DLL_F_EXPORT void ReleaseAI(::CGlobalAI* i)
{
	delete (geniusai::CGeniusAI*)i;
}