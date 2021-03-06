// -------------------------------------------------------------------------
//    @FileName			:    NFCPlayerRedisModule.cpp
//    @Author           :    LvSheng.Huang
//    @Date             :    2013-10-03
//    @Module           :    NFCPlayerRedisModule
//    @Desc             :
// -------------------------------------------------------------------------
#include "NFCPlayerRedisModule.h"

NFCPlayerRedisModule::NFCPlayerRedisModule(NFIPluginManager * p)
{
	pPluginManager = p;
}

bool NFCPlayerRedisModule::Init()
{
	m_pLogicClassModule = pPluginManager->FindModule<NFIClassModule>();
	m_pNoSqlModule = pPluginManager->FindModule<NFINoSqlModule>();
	m_pCommonRedisModule = pPluginManager->FindModule<NFICommonRedisModule>();
	m_pKernelModule = pPluginManager->FindModule<NFIKernelModule>();
	m_pLogModule = pPluginManager->FindModule<NFILogModule>();
	
	m_pKernelModule->RegisterCommonPropertyEvent(this, &NFCPlayerRedisModule::OnPropertyCommonEvent);
	m_pKernelModule->RegisterCommonRecordEvent(this, &NFCPlayerRedisModule::OnRecordCommonEvent);

	return true;
}

int NFCPlayerRedisModule::OnPropertyCommonEvent(const NFGUID & self, const std::string & strPropertyName, const NFData & oldVar, const NFData & newVar)
{
	const std::string& strClassName = m_pKernelModule->GetPropertyString(self, NFrame::IObject::ClassName());
	if (strClassName == NFrame::Player::ThisName())
	{
		NF_SHARE_PTR<NFIObject> pObject = m_pKernelModule->GetObject(self);
		if (pObject)
		{
			NF_SHARE_PTR<NFIPropertyManager> pPropertyManager = pObject->GetPropertyManager();
			if (pPropertyManager)
			{
				NF_SHARE_PTR<NFIProperty> pPropertyInfo = pPropertyManager->GetElement(strPropertyName);
				if (pPropertyInfo)
				{
					if (pPropertyInfo->GetForce())
					{
						//save data with real-time
					}
				}
			}
		}
		
	}

	return 0;
}

int NFCPlayerRedisModule::OnRecordCommonEvent(const NFGUID & self, const RECORD_EVENT_DATA & xEventData, const NFData & oldVar, const NFData & newVar)
{
	const std::string& strClassName = m_pKernelModule->GetPropertyString(self, NFrame::IObject::ClassName());
	if (strClassName == NFrame::Player::ThisName())
	{
		NF_SHARE_PTR<NFIObject> pObject = m_pKernelModule->GetObject(self);
		if (pObject)
		{
			NF_SHARE_PTR<NFIRecordManager> pRecordManager = pObject->GetRecordManager();
			if (pRecordManager)
			{
				NF_SHARE_PTR<NFIRecord> pRecordInfo = pRecordManager->GetElement(xEventData.strRecordName);
				if (pRecordInfo)
				{
					if (pRecordInfo->GetForce())
					{
						//save data with real-time
					}
				}
			}
		}

	}

	return 0;
}

bool NFCPlayerRedisModule::Shut()
{
	return true;
}

bool NFCPlayerRedisModule::Execute()
{
	return true;
}

bool NFCPlayerRedisModule::AfterInit()
{
	m_pKernelModule->AddClassCallBack(NFrame::Player::ThisName(), this, &NFCPlayerRedisModule::OnObjectPlayerEvent);
	m_pKernelModule->AddClassCallBack(NFrame::Guild::ThisName(), this, &NFCPlayerRedisModule::OnObjectGuildEvent);

	return true;
}

bool NFCPlayerRedisModule::LoadPlayerData(const NFGUID & self)
{
	mxObjectDataCache.RemoveElement(self);

	m_pLogModule->LogNormal(NFILogModule::NF_LOG_LEVEL::NLL_DEBUG_NORMAL, self, "Start to load data ", NFGetTimeMS());

	NF_SHARE_PTR<PlayerDataCache> xPlayerDataCache(NF_NEW PlayerDataCache());
	mxObjectDataCache.AddElement(self, xPlayerDataCache);

	m_pCommonRedisModule->GetCachePropertyInfo(self, NFrame::Player::ThisName(), xPlayerDataCache->mvPropertyKeyList, xPlayerDataCache->mvPropertyValueList);
	m_pCommonRedisModule->GetCacheRecordInfo(self, NFrame::Player::ThisName(), xPlayerDataCache->mvRecordKeyList, xPlayerDataCache->mvRecordValueList);

	//xPlayerDataCache->nHomeSceneID = xPlayerDataCache->xPropertyManager->GetPropertyInt(NFrame::Player::HomeSceneID());
	for (int i = 0; i < xPlayerDataCache->mvPropertyKeyList.size(); ++i)
	{
		if (xPlayerDataCache->mvPropertyKeyList[i] == NFrame::Player::HomeSceneID())
		{
			const std::string& strValue = xPlayerDataCache->mvPropertyValueList[i];
			xPlayerDataCache->nHomeSceneID = lexical_cast<int>(strValue);
			break;
		}
	}

	m_pLogModule->LogNormal(NFILogModule::NF_LOG_LEVEL::NLL_DEBUG_NORMAL, self, "loaded data ", NFGetTimeMS());


	return true;
}

int NFCPlayerRedisModule::GetPlayerHomeSceneID(const NFGUID & self)
{
	NF_SHARE_PTR<PlayerDataCache> xPlayerDataCache = mxObjectDataCache.GetElement(self);
	if (xPlayerDataCache)
	{
		return xPlayerDataCache->nHomeSceneID;
	}

	return 0;
}

bool NFCPlayerRedisModule::SavePlayerTile(const int nSceneID, const NFGUID & self, const std::string & strTileData)
{
	std::string strTileKey = m_pCommonRedisModule->GetTileCacheKey(nSceneID);
	NF_SHARE_PTR<NFINoSqlDriver> xNoSqlDriver = m_pNoSqlModule->GetDriverBySuit(self.ToString());
	if (xNoSqlDriver)
	{
		return xNoSqlDriver->HSet(strTileKey, self.ToString(), strTileData);
	}

	return false;
}

bool NFCPlayerRedisModule::LoadPlayerTile(const int nSceneID, const NFGUID & self, std::string & strTileData)
{
	std::string strTileKey = m_pCommonRedisModule->GetTileCacheKey(nSceneID);
	NF_SHARE_PTR<NFINoSqlDriver> xNoSqlDriver = m_pNoSqlModule->GetDriverBySuit(self.ToString());
	if (xNoSqlDriver && xNoSqlDriver->Exists(strTileKey))
	{
		return xNoSqlDriver->HGet(strTileKey, self.ToString(), strTileData);
	}

	return false;
}

bool NFCPlayerRedisModule::LoadPlayerTileRandom(const int nSceneID, NFGUID& xPlayer, std::string & strTileData)
{
	std::string strTileKey = m_pCommonRedisModule->GetTileCacheKey(nSceneID);
	NF_SHARE_PTR<NFINoSqlDriver> xNoSqlDriver = m_pNoSqlModule->GetDriverBySuit(xPlayer.ToString());
	if (xNoSqlDriver && xNoSqlDriver->Exists(strTileKey))
	{
		//need to cache this keys
		std::vector<std::string> vKeys;
		if (xNoSqlDriver->HKeys(strTileKey, vKeys))
		{
			int nKeyIndex = m_pKernelModule->Random(0, (int)vKeys.size());
			std::string strKey = vKeys[nKeyIndex];
			if (xPlayer.FromString(strKey) && xNoSqlDriver->HGet(strTileKey, strKey, strTileData))
			{
				if (mxObjectTileCache.ExistElement(xPlayer))
				{
					mxObjectTileCache.RemoveElement(xPlayer);
				}

				mxObjectTileCache.AddElement(xPlayer, NF_SHARE_PTR<std::string>(NF_NEW std::string(strTileData)));

				return true;
			}
		}
	}

	return false;
}

bool NFCPlayerRedisModule::LoadPlayerTileRandomCache(const NFGUID & xPlayer, std::string & strTileData)
{
	if (mxObjectTileCache.ExistElement(xPlayer))
	{
		NF_SHARE_PTR<std::string> xData = mxObjectTileCache.GetElement(xPlayer);
		if (xData)
		{
			strTileData = *xData;
		}

		return true;
	}

	return false;
}

NFINT64 NFCPlayerRedisModule::GetPropertyInt(const NFGUID & self, const std::string & strPropertyName)
{
	NF_SHARE_PTR<NFINoSqlDriver> pDriver = m_pNoSqlModule->GetDriverBySuit(self.ToString());
	if (!pDriver)
	{
		return 0;
	}

	std::string strValue;
	std::string strCacheKey = m_pCommonRedisModule->GetPropertyCacheKey(self);
	if (!pDriver->HGet(strCacheKey, strPropertyName, strValue))
	{
		return 0;
	}

	return lexical_cast<NFINT64>(strValue);
}

int NFCPlayerRedisModule::GetPropertyInt32(const NFGUID & self, const std::string & strPropertyName)
{
	NF_SHARE_PTR<NFINoSqlDriver> pDriver = m_pNoSqlModule->GetDriverBySuit(self.ToString());
	if (!pDriver)
	{
		return 0;
	}

	std::string strValue;
	std::string strCacheKey = m_pCommonRedisModule->GetPropertyCacheKey(self);
	if (!pDriver->HGet(strCacheKey, strPropertyName, strValue))
	{
		return 0;
	}

	return lexical_cast<NFINT64>(strValue);
}

double NFCPlayerRedisModule::GetPropertyFloat(const NFGUID & self, const std::string & strPropertyName)
{
	NF_SHARE_PTR<NFINoSqlDriver> pDriver = m_pNoSqlModule->GetDriverBySuit(self.ToString());
	if (!pDriver)
	{
		return 0;
	}

	std::string strValue;
	std::string strCacheKey = m_pCommonRedisModule->GetPropertyCacheKey(self);
	if (!pDriver->HGet(strCacheKey, strPropertyName, strValue))
	{
		return 0;
	}

	return lexical_cast<double>(strValue);
}

std::string NFCPlayerRedisModule::GetPropertyString(const NFGUID & self, const std::string & strPropertyName)
{
	NF_SHARE_PTR<NFINoSqlDriver> pDriver = m_pNoSqlModule->GetDriverBySuit(self.ToString());
	if (!pDriver)
	{
		return "";
	}

	std::string strValue;
	std::string strCacheKey = m_pCommonRedisModule->GetPropertyCacheKey(self);
	if (!pDriver->HGet(strCacheKey, strPropertyName, strValue))
	{
		return "";
	}

	return strValue;
}

NFGUID NFCPlayerRedisModule::GetPropertyObject(const NFGUID & self, const std::string & strPropertyName)
{
	NF_SHARE_PTR<NFINoSqlDriver> pDriver = m_pNoSqlModule->GetDriverBySuit(self.ToString());
	if (!pDriver)
	{
		return NFGUID();
	}

	std::string strValue;
	std::string strCacheKey = m_pCommonRedisModule->GetPropertyCacheKey(self);
	if (!pDriver->HGet(strCacheKey, strPropertyName, strValue))
	{
		return NFGUID();
	}
	NFGUID xID;
	xID.FromString(strValue);
	return xID;
}

NFVector2 NFCPlayerRedisModule::GetPropertyVector2(const NFGUID & self, const std::string & strPropertyName)
{
	NF_SHARE_PTR<NFINoSqlDriver> pDriver = m_pNoSqlModule->GetDriverBySuit(self.ToString());
	if (!pDriver)
	{
		return NFVector2();
	}

	std::string strValue;
	std::string strCacheKey = m_pCommonRedisModule->GetPropertyCacheKey(self);
	if (!pDriver->HGet(strCacheKey, strPropertyName, strValue))
	{
		return NFVector2();
	}

	NFVector2 xValue;
	xValue.FromString(strValue);
	return xValue;
}

NFVector3 NFCPlayerRedisModule::GetPropertyVector3(const NFGUID & self, const std::string & strPropertyName)
{
	NF_SHARE_PTR<NFINoSqlDriver> pDriver = m_pNoSqlModule->GetDriverBySuit(self.ToString());
	if (!pDriver)
	{
		return NFVector3();
	}

	std::string strValue;
	std::string strCacheKey = m_pCommonRedisModule->GetPropertyCacheKey(self);
	if (!pDriver->HGet(strCacheKey, strPropertyName, strValue))
	{
		return NFVector3();
	}

	NFVector3 xValue;
	xValue.FromString(strValue);
	return xValue;
}

bool NFCPlayerRedisModule::SavePlayerData(const NFGUID & self)
{
	m_pCommonRedisModule->SaveCachePropertyInfo(self, m_pKernelModule->GetObject(self)->GetPropertyManager());
	m_pCommonRedisModule->SaveCacheRecordInfo(self, m_pKernelModule->GetObject(self)->GetRecordManager());
	
	return true;
}

std::string NFCPlayerRedisModule::GetOnlineGameServerKey()
{
	//if (strValue == "**nonexistent-key**")
	return "OnlineGameKey";
}

std::string NFCPlayerRedisModule::GetOnlineProxyServerKey()
{
	return "OnlineProxyKey";
}

const bool NFCPlayerRedisModule::AttachData(const NFGUID & self)
{
	NF_SHARE_PTR<PlayerDataCache> xPlayerDataCache = mxObjectDataCache.GetElement(self);
	if (xPlayerDataCache)
	{
		//way 1:load first then create object, especially we have loaded nosql plugin
		NF_SHARE_PTR<NFIObject> pObject = m_pKernelModule->GetObject(self);
		if (pObject)
		{
			m_pCommonRedisModule->ConvertPBToPropertyManager(xPlayerDataCache->mvPropertyKeyList, xPlayerDataCache->mvPropertyValueList, pObject->GetPropertyManager());
			m_pCommonRedisModule->ConvertPBToRecordManager(xPlayerDataCache->mvRecordKeyList, xPlayerDataCache->mvRecordValueList, pObject->GetRecordManager());

			return true;
		}
	}
	else
	{
		//way 2:load data when creating a object, especially we donot loaded any sql or nosql plugin
	}

	return false;
}

int NFCPlayerRedisModule::OnObjectPlayerEvent(const NFGUID & self, const std::string & strClassName, const CLASS_OBJECT_EVENT eClassEvent, const NFDataList & var)
{
	if (CLASS_OBJECT_EVENT::COE_DESTROY == eClassEvent)
	{
		OnOffline(self);
		
		m_pLogModule->LogNormal(NFILogModule::NF_LOG_LEVEL::NLL_INFO_NORMAL, self, "start to save data", NFGetTimeMS());

		SavePlayerData(self);

		m_pLogModule->LogNormal(NFILogModule::NF_LOG_LEVEL::NLL_INFO_NORMAL, self, "saved data", NFGetTimeMS());
	}
	else if (CLASS_OBJECT_EVENT::COE_CREATE_LOADDATA == eClassEvent)
	{
		OnOnline(self);

		m_pLogModule->LogNormal(NFILogModule::NF_LOG_LEVEL::NLL_INFO_NORMAL, self, "start to attach data", NFGetTimeMS());

		AttachData(self);

		m_pLogModule->LogNormal(NFILogModule::NF_LOG_LEVEL::NLL_INFO_NORMAL, self, "attached data", NFGetTimeMS());
	}
	else if (CLASS_OBJECT_EVENT::COE_CREATE_FINISH == eClassEvent)
	{
		mxObjectDataCache.RemoveElement(self);
	}

	return 0;
}

int NFCPlayerRedisModule::OnObjectGuildEvent(const NFGUID & self, const std::string & strClassName, const CLASS_OBJECT_EVENT eClassEvent, const NFDataList & var)
{
	if (CLASS_OBJECT_EVENT::COE_DESTROY == eClassEvent)
	{
		SavePlayerData(self);
	}
	else if (CLASS_OBJECT_EVENT::COE_CREATE_LOADDATA == eClassEvent)
	{
		AttachData(self);
	}
	else if (CLASS_OBJECT_EVENT::COE_CREATE_FINISH == eClassEvent)
	{
		mxObjectDataCache.RemoveElement(self);
	}

	return 0;
}

void NFCPlayerRedisModule::OnOnline(const NFGUID & self)
{
	//const int nGateID = m_pKernelModule->GetPropertyInt32(self, NFrame::Player::GateID());
	//const int nServerID = m_pKernelModule->GetPropertyInt32(self, NFrame::Player::GameID());
}

void NFCPlayerRedisModule::OnOffline(const NFGUID & self)
{
}
