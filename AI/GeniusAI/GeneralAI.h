#ifndef GENERAL_AI_H
#define GENERAL_AI_H

#include "Common.h"

namespace geniusai {
namespace generalai {

class CGeneralAI {
public:
	CGeneralAI();
	~CGeneralAI();

	void init(ICallback* call_back);
private:
	ICallback* call_back_;
};

}}
#endif // GENERAL_AI_H