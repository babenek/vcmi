#include "StdInc.h"
#include "RegisterTypes.h"

#include "../mapping/CMapInfo.h"
#include "../StartInfo.h"
#include "../BattleState.h"
#include "../CGameState.h"
#include "../mapping/CMap.h"
#include "../CModHandler.h"
#include "../CObjectHandler.h"
#include "../CCreatureHandler.h"
#include "../VCMI_Lib.h"
#include "../CArtHandler.h"
#include "../CHeroHandler.h"
#include "../CSpellHandler.h"
#include "../CTownHandler.h"
#include "../mapping/CCampaignHandler.h"
#include "../NetPacks.h"
#include "../CDefObjInfoHandler.h"


template void registerTypesMapObjects1<CISer<CConnection>>(CISer<CConnection>& s);
template void registerTypesMapObjects1<COSer<CConnection>>(COSer<CConnection>& s);
template void registerTypesMapObjects1<CISer<CMemorySerializer>>(CISer<CMemorySerializer>& s);
template void registerTypesMapObjects1<COSer<CMemorySerializer>>(COSer<CMemorySerializer>& s);
template void registerTypesMapObjects1<CSaveFile>(CSaveFile & s);
template void registerTypesMapObjects1<CLoadFile>(CLoadFile & s);
template void registerTypesMapObjects1<CTypeList>(CTypeList & s);
template void registerTypesMapObjects1<CLoadIntegrityValidator>(CLoadIntegrityValidator & s);

