#ifndef GENERAL_AI_H
#define GENERAL_AI_H

#pragma warning(push, 0)
#include "Common.h"
#pragma warning(pop)

namespace geniusai {
namespace generalai {
class CGeneralAI
{
public:
	CGeneralAI();
	~CGeneralAI();

	void init(ICallback* call_back);
private:
	ICallback* call_back_;
};
}}

#endif // GENERAL_AI_H