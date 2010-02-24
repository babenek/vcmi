#include "StdAfx.h"
#include "GeneralAI.h"

#pragma warning(push, 0)
#include "../../CCallback.h"
#pragma warning(pop)

namespace geniusai {
namespace generalai {
CGeneralAI::CGeneralAI()
	: call_back_(NULL)
{
}

CGeneralAI::~CGeneralAI()
{
}

void CGeneralAI::init(::ICallback* call_back)
{
	assert(call_back != NULL);
	call_back_ = call_back;
	call_back->waitTillRealize = true;
}
}}