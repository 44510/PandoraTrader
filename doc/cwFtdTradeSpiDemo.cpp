#include "cwFtdTradeSpiDemo.h"
#include <float.h>
#include <time.h>
#include <sys/stat.h>
#include <math.h>
#include <functional>
#ifdef _MSC_VER
#include <direct.h>
#else
#include <unistd.h>
#endif

#ifndef MAX_PATH
#define MAX_PATH          260
#endif // !MAX_PATH


#define DISCONNECT_SOUND

cwFtdTradeSpiDemo::cwFtdTradeSpiDemo()
	: cwBasicTradeSpi(cwTradeAPIType::cwTrade_CTP)
#ifdef CW_USING_DYNAMIC_LOADING_DLL
	, m_cwhDllInstance(NULL)
#endif
	, m_pTraderUserApi(NULL)
	, m_iRequestId(0)
	, m_bReqQryThreadRun(true)
	, m_iResumeType(THOST_TERT_QUICK)
{
	memset(&m_ReqUserLoginField, 0, sizeof(CThostFtdcReqUserLoginField));
	memset(m_szTradeFrount, 0, sizeof(m_szTradeFrount));

	m_iTradeAPIIndex = m_iTradeApiCount;
}

cwFtdTradeSpiDemo::cwFtdTradeSpiDemo(const char * pLogFileName)
	: cwBasicTradeSpi(cwTradeAPIType::cwTrade_CTP, pLogFileName)
#ifdef CW_USING_DYNAMIC_LOADING_DLL
	, m_cwhDllInstance(NULL)
#endif
	, m_pTraderUserApi(NULL)
	, m_iRequestId(0)
	, m_bReqQryThreadRun(true)
	, m_iResumeType(THOST_TERT_QUICK)
{
	memset(&m_ReqUserLoginField, 0, sizeof(CThostFtdcReqUserLoginField));
	memset(m_szTradeFrount, 0, sizeof(m_szTradeFrount));

	m_iTradeAPIIndex = m_iTradeApiCount;
}


cwFtdTradeSpiDemo::~cwFtdTradeSpiDemo()
{
	DisConnect();

#ifdef CW_USING_DYNAMIC_LOADING_DLL
	if (m_cwhDllInstance != NULL)
	{
#ifdef _MSC_VER
		FreeLibrary(m_cwhDllInstance);
#else
		dlclose(m_cwhDllInstance);
#endif
		m_cwhDllInstance = NULL;
	}
#endif //CW_USING_DYNAMIC_LOADING_DLL
}

void cwFtdTradeSpiDemo::OnFrontConnected()
{
	cwBasicTradeSpi::Reset();

	m_CurrentStatus = TradeServerStatus::Status_Connected;

#ifdef CWCOUTINFO
	m_cwShow.AddLog("%s Trade: OnFrontConnected", g_cwGetTradeApiName(m_cwTradeAPIType));
	//std::cout << "Trade: OnFrontConnected" << std::endl;
#endif

	CThostFtdcReqAuthenticateField cwQryFild;

#ifdef _MSC_VER
	ZeroMemory(&cwQryFild, sizeof(CThostFtdcReqAuthenticateField));
	strncpy_s(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
	strncpy_s(cwQryFild.UserID, m_ReqUserLoginField.UserID, sizeof(cwQryFild.UserID));
	strncpy_s(cwQryFild.UserProductInfo, m_ReqUserLoginField.UserProductInfo, sizeof(cwQryFild.UserProductInfo));
	strncpy_s(cwQryFild.AppID, m_AppID, sizeof(cwQryFild.AppID));
	strncpy_s(cwQryFild.AuthCode, m_AuthCode, sizeof(cwQryFild.AuthCode));
#else
	memset(&cwQryFild, 0, sizeof(CThostFtdcQryInvestorField));
	strncpy(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
	strncpy(cwQryFild.UserID, m_ReqUserLoginField.UserID, sizeof(cwQryFild.UserID));
	strncpy(cwQryFild.UserProductInfo, m_ReqUserLoginField.UserProductInfo, sizeof(cwQryFild.UserProductInfo));
	strncpy(cwQryFild.AppID, m_AppID, sizeof(cwQryFild.AppID));
	strncpy(cwQryFild.AuthCode, m_AuthCode, sizeof(cwQryFild.AuthCode));
#endif // _MSC_VER

	MyReqFunction(cwReqAuthenticate, (void *)(&cwQryFild));

}

#ifdef DISCONNECT_SOUND
#ifdef _MSC_VER
#include <MMSystem.h>
//#include <playsoundapi.h>  
#pragma comment(lib, "Winmm.lib") 
#endif
#endif

void cwFtdTradeSpiDemo::OnFrontDisconnected(int nReason)
{
#ifdef CWCOUTINFO
	m_cwShow.AddLog("%s Trade: OnFrontDisconnected!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!", g_cwGetTradeApiName(m_cwTradeAPIType));
	//std::cout << "Trade: OnFrontDisconnected!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
#endif
	m_bReqQryThreadRun = false;
	
	m_CurrentStatus = TradeServerStatus::Status_UnConnected;

#ifdef TRADELOG
	std::string strMsg;
	strMsg = ",,,,,,,,,,,,,onDisConnect:";
	strMsg.append(m_ReqUserLoginField.UserID);

	m_TradeLog.AddLog(cwTradeLog::enMsg, strMsg.c_str(), true);

#endif // TRADELOG

	//DisConnect();

#ifdef DISCONNECT_SOUND
#ifdef _MSC_VER
	PlaySound(TEXT("./Sound/Ding.wav"), NULL, SND_FILENAME | SND_ASYNC);
	cwSleep(3000);
#endif
#endif

	if (m_bDisConnectExit)
	{
		exit(1);
	}
}

void cwFtdTradeSpiDemo::OnRspAuthenticate(CThostFtdcRspAuthenticateField * pRspAuthenticateField, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo == NULL)
	{
		return;
	}

#ifdef TRADELOG
	{
		std::string strMsg;
		strMsg = ",,,,,,,,,,,,,Authenticate:";
		strMsg.append(pRspInfo->ErrorMsg);

		m_TradeLog.AddLog(cwTradeLog::enMsg, strMsg.c_str());
	}
#endif // TRADELOG

	if (pRspInfo->ErrorID == 0 && nRequestID == m_iRequestId)
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi:OnRspAuthenticate %s BrokerID:%s UserID:%s UserProductInfo:%s",
			pRspInfo->ErrorMsg, pRspAuthenticateField->BrokerID, pRspAuthenticateField->UserID, pRspAuthenticateField->UserProductInfo);
		//std::cout << "TradeSpi:OnRspAuthenticate " 
		//	<< "BrokerID " << pRspAuthenticateField->BrokerID << " "
		//	<< "UserID " << pRspAuthenticateField->UserID << " "
		//	<< "UserProductInfo " << pRspAuthenticateField->UserProductInfo <<std::endl;
#endif
		if (bIsLast)
		{
			cwSleep(1000);

			MyReqFunction(cwReqUserLogin, (void *)(&m_ReqUserLoginField));
		}
	}
	else
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi:OnRspAuthenticate ErrorID:%d %s", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		//std::cout << "TradeSpi:OnRspAuthenticate "
		//	<< "ErrorID " << pRspInfo->ErrorID << " "
		//	<< pRspInfo->ErrorMsg << std::endl;
#endif
	}
}

void cwFtdTradeSpiDemo::OnRspUserLogin(CThostFtdcRspUserLoginField * pRspUserLogin, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo == NULL)
	{
		return;
	}

#ifdef TRADELOG
	{
		std::string strMsg;
		strMsg = ",,,,,,,,,,,,,";
		strMsg.append(m_ReqUserLoginField.UserID);
		strMsg.append(" Login:");
		strMsg.append(pRspInfo->ErrorMsg);
		m_TradeLog.AddLog(cwTradeLog::enMsg, strMsg.c_str());
	}
#endif // TRADELOG

	if (pRspInfo->ErrorID == 0 && nRequestID == m_iRequestId)
	{
		m_CurrentStatus = TradeServerStatus::Status_Logined;

		m_SessionID = pRspUserLogin->SessionID;
		m_FrontID = pRspUserLogin->FrontID;

		m_cwOrderRef.UpdateOrderRef(pRspUserLogin->MaxOrderRef);
	
#ifdef _MSC_VER
		strncpy_s(m_cwTradeLoginTime, pRspUserLogin->LoginTime, sizeof(m_cwTradeLoginTime));
		strncpy_s(m_cwTradeLoginTradingDay, pRspUserLogin->TradingDay, sizeof(m_cwTradeLoginTradingDay));
#else
		strncpy(m_cwTradeLoginTime, pRspUserLogin->LoginTime, sizeof(m_cwTradeLoginTime));
		strncpy(m_cwTradeLoginTradingDay, pRspUserLogin->TradingDay, sizeof(m_cwTradeLoginTradingDay));
#endif

#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi:OnRspUserLogin %s %s m_SessionID:%d m_FrontID:%d MaxOrderRef:%s LoginTime:%s",
			pRspInfo->ErrorMsg, pRspUserLogin->SystemName, pRspUserLogin->SessionID, pRspUserLogin->FrontID, pRspUserLogin->MaxOrderRef, pRspUserLogin->LoginTime);
		//std::cout << "TradeSpi:OnRspUserLogin " 
		//	<< "m_SessionID " << pRspUserLogin->SessionID << " "
		//	<< "m_FrontID " << pRspUserLogin->FrontID << " "
		//	<< "MaxOrderRef " << pRspUserLogin->MaxOrderRef <<std::endl;
#endif
		if (bIsLast)
		{
			CThostFtdcQryInvestorField cwQryFild;
#ifdef _MSC_VER
			ZeroMemory(&cwQryFild, sizeof(CThostFtdcQryInvestorField));
			strncpy_s(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
#else
			memset(&cwQryFild, 0, sizeof(CThostFtdcQryInvestorField));
			strncpy(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
#endif // _MSC_VER

			cwSleep(1000);

			MyReqFunction(cwReqQryInvestor, (void *)(&cwQryFild));
		}
	}
	else
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi:OnRspUserLogin ErrorID:%d %s", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
		//std::cout << "TradeSpi:OnRspUserLogin "
		//	<< "ErrorID " << pRspInfo->ErrorID << " "
		//	<< pRspInfo->ErrorMsg << std::endl;
#endif
	}
}

void cwFtdTradeSpiDemo::OnRspQryInvestor(CThostFtdcInvestorField * pInvestor, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (nRequestID == m_iRequestId)
	{
		if (pInvestor)
		{
			m_CurrentStatus = TradeServerStatus::Status_Initial;

			m_strInvestorID = pInvestor->InvestorID;
			m_strInvestorName = pInvestor->InvestorName;

#ifdef CWCOUTINFO
			m_cwShow.AddLog("TradeSpi: %s m_strInvestorID:%s", pInvestor->InvestorName, pInvestor->InvestorID);
			//std::cout << "TradeSpi: m_strInvestorID:" << pInvestor->InvestorID << std::endl;
#endif

		}

		if (m_CurrentStatus == TradeServerStatus::Status_Initial && bIsLast)
		{
			CThostFtdcQryInvestorPositionField cwQryFild;
#ifdef _MSC_VER
			ZeroMemory(&cwQryFild, sizeof(CThostFtdcQryInvestorPositionField));
			strncpy_s(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
			strncpy_s(cwQryFild.InvestorID, m_strInvestorID.c_str(), m_strInvestorID.size());
#else
			memset(&cwQryFild, 0, sizeof(CThostFtdcQryInvestorPositionField));
			strncpy(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
			strncpy(cwQryFild.InvestorID, m_strInvestorID.c_str(), m_strInvestorID.size());
#endif // _MSC_VER

			cwSleep(1000);

			MyReqFunction(cwRspQryInvestorPosition, (void *)(&cwQryFild));
		}
	}
}

void cwFtdTradeSpiDemo::OnRspQryInvestorPosition(CThostFtdcInvestorPositionField * pInvestorPosition, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
#ifdef TRADELOG
	if (pInvestorPosition != NULL)
	{
		std::string OrderMsg;
		OrderMsg = pInvestorPosition->InstrumentID;
		OrderMsg.append(",N/A,N/A,N/A,");
		OrderMsg.append(std::to_string(pInvestorPosition->YdPosition));
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pInvestorPosition->Position));
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pInvestorPosition->TodayPosition));
		if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long)
		{
			OrderMsg.append(",L,");
		}
		else
		{
			OrderMsg.append(",S,");
		}
		switch (pInvestorPosition->PositionDate)
		{
		case THOST_FTDC_PSD_Today:
			OrderMsg.append("Today,");
			break;
			///ƽ��
		case THOST_FTDC_PSD_History:
			OrderMsg.append("History,");
			break;
		default:
			OrderMsg.append(",");
			break;
		}
		OrderMsg.append("N/A,N/A,N/A,N/A,N/A");
		m_TradeLog.AddLog(cwTradeLog::enRP, OrderMsg.c_str(), bIsLast);
	}
#endif
	m_bHasPositionChanged = true;
	if ((!m_bHasGetPosition) && pInvestorPosition != NULL)
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("Position: %s %s %d %s",
			pInvestorPosition->InstrumentID,
			(pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long ? " Long " : " Short "),
			pInvestorPosition->Position,
			(pInvestorPosition->PositionDate == THOST_FTDC_PSD_Today ? " Today " : " History "));
		//std::cout <<"Position: " << pInvestorPosition->InstrumentID 
		//	<< (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long ? " Long " : " Short ") 
		//	<< pInvestorPosition->Position 
		//	<< (pInvestorPosition->PositionDate == THOST_FTDC_PSD_Today ? " Today " : " History ")<< std::endl;
#endif

		const std::string InstrumentID = pInvestorPosition->InstrumentID;
		std::map<std::string, cwPositionPtr>::iterator it;
		it = m_PositionTempMap.find(InstrumentID);
		if (it == m_PositionTempMap.end())
		{
			cwPositionPtr newPos(new cwPositionField);

			m_PositionTempMap.insert(std::pair<std::string, cwPositionPtr>(InstrumentID, newPos));
			it = m_PositionTempMap.find(InstrumentID);
		}

		{
			if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Long)
			{
				it->second->LongPosition->UpdatePosition(pInvestorPosition->InstrumentID, GetCtp2CwDirectionType(pInvestorPosition->PosiDirection, true), GetCtp2CwHedgeFlagType(pInvestorPosition->HedgeFlag),
					pInvestorPosition->YdPosition, pInvestorPosition->TodayPosition, pInvestorPosition->Position,
					pInvestorPosition->LongFrozen, pInvestorPosition->ShortFrozen, pInvestorPosition->PositionCost,
					pInvestorPosition->OpenCost, pInvestorPosition->UseMargin,
					pInvestorPosition->PositionProfit, pInvestorPosition->CloseProfitByDate, pInvestorPosition->CloseProfitByTrade,
					pInvestorPosition->MarginRateByMoney, pInvestorPosition->MarginRateByVolume, pInvestorPosition->PositionDate);
			}
			if (pInvestorPosition->PosiDirection == THOST_FTDC_PD_Short)
			{
				it->second->ShortPosition->UpdatePosition(pInvestorPosition->InstrumentID, GetCtp2CwDirectionType(pInvestorPosition->PosiDirection, true), GetCtp2CwHedgeFlagType(pInvestorPosition->HedgeFlag),
					pInvestorPosition->YdPosition, pInvestorPosition->TodayPosition, pInvestorPosition->Position,
					pInvestorPosition->LongFrozen, pInvestorPosition->ShortFrozen, pInvestorPosition->PositionCost,
					pInvestorPosition->OpenCost, pInvestorPosition->UseMargin,
					pInvestorPosition->PositionProfit, pInvestorPosition->CloseProfitByDate, pInvestorPosition->CloseProfitByTrade,
					pInvestorPosition->MarginRateByMoney, pInvestorPosition->MarginRateByVolume, pInvestorPosition->PositionDate);
			}
		}

	}

	if (bIsLast && m_CurrentStatus == TradeServerStatus::Status_Initial)
	{
		CThostFtdcQryInvestorPositionDetailField cwQryFild;
#ifdef _MSC_VER
		ZeroMemory(&cwQryFild, sizeof(CThostFtdcQryInvestorPositionDetailField));
		strncpy_s(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
		strncpy_s(cwQryFild.InvestorID, m_strInvestorID.c_str(), m_strInvestorID.size());
#else
		memset(&cwQryFild, 0, sizeof(CThostFtdcQryInvestorPositionDetailField));
		strncpy(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
		strncpy(cwQryFild.InvestorID, m_strInvestorID.c_str(), m_strInvestorID.size());
#endif

		cwSleep(1000);

		MyReqFunction(cwRspQryInvestorPositionDetail, (void *)(&cwQryFild));
	}
}

void cwFtdTradeSpiDemo::OnRspQryInvestorPositionDetail(CThostFtdcInvestorPositionDetailField * pInvestorPositionDetail, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pInvestorPositionDetail != NULL)
	{
		const std::string InstrumentID = pInvestorPositionDetail->InstrumentID;
		std::map<std::string, cwPositionPtr>::iterator it;
		it = m_PositionTempMap.find(InstrumentID);
		if (it == m_PositionTempMap.end())
		{
			cwPositionPtr newPos(new cwPositionField);

			m_PositionTempMap.insert(std::pair<std::string, cwPositionPtr>(InstrumentID, newPos));
			it = m_PositionTempMap.find(InstrumentID);
		}

		{
			if (pInvestorPositionDetail->Direction == THOST_FTDC_D_Buy
				&& it->second->LongPosition->TotalPosition != 0)
			{
				for (int n = 0; n < pInvestorPositionDetail->Volume; n++)
				{
					it->second->LongPositionPriceSum += pInvestorPositionDetail->OpenPrice;
					it->second->LongPositionPrice.push_back(pInvestorPositionDetail->OpenPrice);
				}

				it->second->LongPosition->AveragePosPrice = it->second->LongPositionPriceSum / it->second->LongPosition->TotalPosition;
			}

			if (pInvestorPositionDetail->Direction == THOST_FTDC_D_Sell
				&& it->second->ShortPosition->TotalPosition != 0)
			{
				for (int n = 0; n < pInvestorPositionDetail->Volume; n++)
				{
					it->second->ShortPositionPriceSum += pInvestorPositionDetail->OpenPrice;
					it->second->ShortPositionPrice.push_back(pInvestorPositionDetail->OpenPrice);
				}

				it->second->ShortPosition->AveragePosPrice = it->second->ShortPositionPriceSum / it->second->ShortPosition->TotalPosition;
			}
		}
	}

	if ((!m_bHasGetPosition) && bIsLast)
	{
		m_bHasGetPosition = true;

		cwAUTOMUTEX mt(m_TradeSpiMutex, true);
		m_PositionMap = m_PositionTempMap;
		mt.unlock();

		m_PositionTempMap.clear();
	}

	if (bIsLast)
	{
		if (m_InstrumentMap.size() == 0)
		{
			CThostFtdcQryInstrumentField cwQryFild;
#ifdef _MSC_VER
			ZeroMemory(&cwQryFild, sizeof(CThostFtdcQryInstrumentField));
#else
			memset(&cwQryFild, 0, sizeof(CThostFtdcQryInstrumentField));
#endif // _MSC_VER

			MyReqFunction(cwReqQryInstrument, (void *)(&cwQryFild));
		}
		else
		{
			if (!m_bHasGetOrders || m_CurrentStatus == TradeServerStatus::Status_Initial)
			{
				CThostFtdcQryOrderField cwQryFild;
#ifdef _MSC_VER
				ZeroMemory(&cwQryFild, sizeof(CThostFtdcQryOrderField));
				strncpy_s(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
				strncpy_s(cwQryFild.InvestorID, m_strInvestorID.c_str(), m_strInvestorID.size());
#else
				memset(&cwQryFild, 0, sizeof(CThostFtdcQryOrderField));
				strncpy(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
				strncpy(cwQryFild.InvestorID, m_strInvestorID.c_str(), m_strInvestorID.size());
#endif // _MSC_VER

				cwSleep(1000);

				MyReqFunction(cwReqQryOrder, (void *)(&cwQryFild));
			}
		}
	}

}

void cwFtdTradeSpiDemo::OnRspQryInstrument(CThostFtdcInstrumentField * pInstrument, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pInstrument != NULL)
	{
		cwInstrumentDataPtr InsPtr;
		InsPtr.reset(new cwFtdcInstrumentField);
		if (InsPtr.get() == NULL)
		{
			return;
		}

		memset(InsPtr.get(), 0, sizeof(cwFtdcInstrumentField));

#ifdef _MSC_VER
		memcpy_s(InsPtr->ExchangeID, sizeof(InsPtr->ExchangeID), pInstrument->ExchangeID, sizeof(pInstrument->ExchangeID));
		memcpy_s(InsPtr->InstrumentID, sizeof(InsPtr->InstrumentID), pInstrument->InstrumentID, sizeof(pInstrument->InstrumentID));
		memcpy_s(InsPtr->InstrumentName, sizeof(InsPtr->InstrumentName), pInstrument->InstrumentName, sizeof(pInstrument->InstrumentName));
		memcpy_s(InsPtr->ProductID, sizeof(InsPtr->ProductID), pInstrument->ProductID, sizeof(pInstrument->ProductID));
		InsPtr->ProductClass = GetCtp2CwProductClassType(pInstrument->ProductClass);
		memcpy_s(InsPtr->CreateDate, sizeof(InsPtr->CreateDate), pInstrument->CreateDate, sizeof(pInstrument->CreateDate));
		memcpy_s(InsPtr->OpenDate, sizeof(InsPtr->OpenDate), pInstrument->OpenDate, sizeof(pInstrument->OpenDate));
		memcpy_s(InsPtr->ExpireDate, sizeof(InsPtr->ExpireDate), pInstrument->ExpireDate, sizeof(pInstrument->ExpireDate));
		InsPtr->Currency = CW_FTDC_C_RMB;
		InsPtr->OptionsType = pInstrument->OptionsType;
		memcpy_s(InsPtr->StartDelivDate, sizeof(InsPtr->StartDelivDate), pInstrument->StartDelivDate, sizeof(pInstrument->StartDelivDate));
		memcpy_s(InsPtr->EndDelivDate, sizeof(InsPtr->EndDelivDate), pInstrument->EndDelivDate, sizeof(pInstrument->EndDelivDate));
		InsPtr->PositionType = pInstrument->PositionType;
		InsPtr->MaxMarginSideAlgorithm = pInstrument->MaxMarginSideAlgorithm;
		memcpy_s(InsPtr->UnderlyingInstrID, sizeof(InsPtr->UnderlyingInstrID), pInstrument->UnderlyingInstrID, sizeof(pInstrument->UnderlyingInstrID));
		InsPtr->UnderlyingMultiple = pInstrument->UnderlyingMultiple;
		InsPtr->DeliveryYear = pInstrument->DeliveryYear;
		InsPtr->DeliveryMonth = pInstrument->DeliveryMonth;
		InsPtr->MaxMarketOrderVolume = pInstrument->MaxMarketOrderVolume;
		InsPtr->MinMarketOrderVolume = pInstrument->MinMarketOrderVolume;
		InsPtr->MaxLimitOrderVolume = pInstrument->MaxLimitOrderVolume;
		InsPtr->MinLimitOrderVolume = pInstrument->MinLimitOrderVolume;
		InsPtr->IsTrading = pInstrument->IsTrading;
		InsPtr->VolumeMultiple = pInstrument->VolumeMultiple;
		InsPtr->PriceTick = pInstrument->PriceTick;
		InsPtr->StrikePrice = pInstrument->StrikePrice;
#else
		memcpy(InsPtr->ExchangeID, pInstrument->ExchangeID, sizeof(pInstrument->ExchangeID));
		memcpy(InsPtr->InstrumentID, pInstrument->InstrumentID, sizeof(pInstrument->InstrumentID));
		memcpy(InsPtr->InstrumentName, pInstrument->InstrumentName, sizeof(pInstrument->InstrumentName));
		memcpy(InsPtr->ProductID, pInstrument->ProductID, sizeof(pInstrument->ProductID));
		InsPtr->ProductClass = GetCtp2CwProductClassType(pInstrument->ProductClass);
		memcpy(InsPtr->CreateDate, pInstrument->CreateDate, sizeof(pInstrument->CreateDate));
		memcpy(InsPtr->OpenDate, pInstrument->OpenDate, sizeof(pInstrument->OpenDate));
		memcpy(InsPtr->ExpireDate, pInstrument->ExpireDate, sizeof(pInstrument->ExpireDate));
		InsPtr->Currency = CW_FTDC_C_RMB;
		InsPtr->OptionsType = pInstrument->OptionsType;
		memcpy(InsPtr->StartDelivDate, pInstrument->StartDelivDate, sizeof(pInstrument->StartDelivDate));
		memcpy(InsPtr->EndDelivDate, pInstrument->EndDelivDate, sizeof(pInstrument->EndDelivDate));
		InsPtr->PositionType = pInstrument->PositionType;
		InsPtr->MaxMarginSideAlgorithm = pInstrument->MaxMarginSideAlgorithm;
		memcpy(InsPtr->UnderlyingInstrID, pInstrument->UnderlyingInstrID, sizeof(pInstrument->UnderlyingInstrID));
		InsPtr->UnderlyingMultiple = pInstrument->UnderlyingMultiple;
		InsPtr->DeliveryYear = pInstrument->DeliveryYear;
		InsPtr->DeliveryMonth = pInstrument->DeliveryMonth;
		InsPtr->MaxMarketOrderVolume = pInstrument->MaxMarketOrderVolume;
		InsPtr->MinMarketOrderVolume = pInstrument->MinMarketOrderVolume;
		InsPtr->MaxLimitOrderVolume = pInstrument->MaxLimitOrderVolume;
		InsPtr->MinLimitOrderVolume = pInstrument->MinLimitOrderVolume;
		InsPtr->IsTrading = pInstrument->IsTrading;
		InsPtr->VolumeMultiple = pInstrument->VolumeMultiple;
		InsPtr->PriceTick = pInstrument->PriceTick;
		InsPtr->StrikePrice = pInstrument->StrikePrice;
#endif // _MSC_VER

		if (DBL_MAX - InsPtr->StrikePrice\
			<= std::numeric_limits<double>::epsilon())\
		{\
			InsPtr->StrikePrice = 0; \
		}

		std::string strIntrumentID = InsPtr->InstrumentID;
		auto it = m_InstrumentMap.find(strIntrumentID);
		if (it == m_InstrumentMap.end())
		{
			m_InstrumentMap.insert(std::make_pair(strIntrumentID, InsPtr));
		}
		else
		{
			it->second = InsPtr;
		}
	}

	if (bIsLast)
	{
#ifdef  UPDATE_ORDERRANKED
		m_TickTradeManger.SetInstrumentData(m_InstrumentMap);
#endif
		if (m_pBasicStrategy != NULL)
		{
			for (auto it = m_InstrumentMap.begin();
				it != m_InstrumentMap.end(); it++)
			{
				auto Ret = m_pBasicStrategy->m_InstrumentMap.insert(std::make_pair(it->first, it->second));
				if (!Ret.second)
				{
					Ret.first->second = it->second;
				}
			}
		}

		if (m_bSaveInstrumentDataToFile)
		{
			this->GenerateInstrumentDataToFile();
		}

		if (!m_bHasGetOrders || m_CurrentStatus == TradeServerStatus::Status_Initial)
		{
			CThostFtdcQryOrderField cwQryFild;
#ifdef _MSC_VER
			ZeroMemory(&cwQryFild, sizeof(CThostFtdcQryOrderField));
			strncpy_s(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
			strncpy_s(cwQryFild.InvestorID, m_strInvestorID.c_str(), m_strInvestorID.size());
#else
			memset(&cwQryFild, 0, sizeof(CThostFtdcQryOrderField));
			strncpy(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
			strncpy(cwQryFild.InvestorID, m_strInvestorID.c_str(), m_strInvestorID.size());
#endif // _MSC_VER

			cwSleep(1000);

			MyReqFunction(cwReqQryOrder, (void *)(&cwQryFild));
		}
	}
}

void cwFtdTradeSpiDemo::OnRspQryOrder(CThostFtdcOrderField * pOrder, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	m_bHasGetOrders = true;
	m_bHasOrdersChanged = true;
	m_bHasActiveOrdersChanged = true;

	cwOrderPtr OrderPtr = GetcwOrderPtr(pOrder);

	cwAUTOMUTEX mt(m_TradeSpiMutex, false);

	if (pOrder != NULL)
	{
		m_cwOrderRef.UpdateOrderRef(pOrder->OrderRef);

		if (OrderPtr.get() == NULL)
		{
#ifdef CWCOUTINFO
			m_cwShow.AddLog("OnRspQryOrder: malloc Order Memory Failed !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
#endif
			return;
		}

#ifdef CWRISK
		if (!IsIOCTypeOrder(OrderPtr))
		{
			switch (pOrder->OrderStatus)
			{
			case THOST_FTDC_OST_AllTraded:
				///ȫ���ɽ�
			case THOST_FTDC_OST_PartTradedNotQueueing:
				///���ֳɽ����ڶ�����
			case THOST_FTDC_OST_NoTradeNotQueueing:
				///δ�ɽ����ڶ�����
				break;
			case THOST_FTDC_OST_PartTradedQueueing:
				///���ֳɽ����ڶ�����
			case THOST_FTDC_OST_NoTradeQueueing:
				///δ�ɽ����ڶ�����
			case THOST_FTDC_OST_Canceled:
				///����
			{
				auto CancelLimitMapIt = m_iCancelLimitMap.find(pOrder->InstrumentID);
				if (CancelLimitMapIt == m_iCancelLimitMap.end())
				{
					m_iCancelLimitMap.insert(std::pair<std::string, int>(pOrder->InstrumentID, 1));
				}
				else
				{
					mt.lock();
					if (strlen(pOrder->OrderSysID) > 0)
					{
						CancelLimitMapIt->second++;
					}
					mt.unlock();
				}
			}
			break;
			default:
				break;
			}
		}
#endif // CWRISK

		std::map<std::string, cwOrderPtr>::iterator it;
		std::string ExchangeOrderID = pOrder->OrderSysID;

		if (ExchangeOrderID.size() > 1)
		{
			mt.lock();

			if (OrderPtr->CombOffsetFlag[0] == CW_FTDC_OF_Open)
			{
				m_bHasOpenOffsetOrderMap.insert(std::pair<std::string, bool>(OrderPtr->InstrumentID, true));
			}

			it = m_OrdersMap.find(ExchangeOrderID);
			if (it == m_OrdersMap.end())
			{
				m_OrdersMap.insert(std::make_pair(ExchangeOrderID, OrderPtr));
			}
			else
			{
				it->second = OrderPtr;
			}
			mt.unlock();
		}

		//Insert Active Order Map
		switch (pOrder->OrderStatus)
		{
		case THOST_FTDC_OST_AllTraded:
			//ȫ���ɽ�
		case THOST_FTDC_OST_PartTradedNotQueueing:
			///���ֳɽ����ڶ�����
		case THOST_FTDC_OST_NoTradeNotQueueing:
			///δ�ɽ����ڶ�����
		case THOST_FTDC_OST_Canceled:
			///����
			break;
		case THOST_FTDC_OST_PartTradedQueueing:
			///���ֳɽ����ڶ�����
		case THOST_FTDC_OST_NoTradeQueueing:
			///δ�ɽ����ڶ�����
		case THOST_FTDC_OST_Unknown:
			///δ֪
			m_bHasActiveOrdersChanged = true;
			ExchangeOrderID = pOrder->OrderRef;
			mt.lock();
			it = m_ActiveOrdersMap.find(ExchangeOrderID);
			if (it == m_ActiveOrdersMap.end())
			{
				m_ActiveOrdersMap.insert(std::make_pair(ExchangeOrderID, OrderPtr));
			}
			else
			{
				it->second = OrderPtr;
			}
			mt.unlock();
			break;
		default:
			break;
		}

#ifdef TRADELOG
		std::string OrderMsg;
		OrderMsg = pOrder->InstrumentID;
		OrderMsg.append(",");
		OrderMsg.append(pOrder->OrderRef);
		OrderMsg.append(",");
		OrderMsg.append(pOrder->OrderSysID);
		OrderMsg.append(",");
		OrderMsg.append(pOrder->UserProductInfo);
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pOrder->LimitPrice));
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pOrder->VolumeTotalOriginal));
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pOrder->VolumeTraded));
		if (pOrder->Direction == THOST_FTDC_D_Buy)
		{
			OrderMsg.append(",B,");
		}
		else
		{
			OrderMsg.append(",S,");
		}
		switch (pOrder->CombOffsetFlag[0])
		{
		case THOST_FTDC_OF_Open:
			OrderMsg.append("Open,");
			break;
			///ƽ��
		case THOST_FTDC_OF_Close:
			OrderMsg.append("Close,");
			break;
			///ƽ��
		case THOST_FTDC_OF_CloseToday:
			OrderMsg.append("CloseTd,");
			break;
			///ƽ��
		case THOST_FTDC_OF_CloseYesterday:
			OrderMsg.append("CloseYd,");
			break;
		default:
			break;
		}
		OrderMsg.append(GetInsertOrderTypeString(GetInsertOrderType(OrderPtr)));
		OrderMsg.append(",");
		OrderMsg.append(pOrder->InsertTime);
		OrderMsg.append(",");
		OrderMsg.append(pOrder->CancelTime);
		OrderMsg.append(",");
		switch (pOrder->OrderStatus)
		{
		case THOST_FTDC_OST_AllTraded:
			OrderMsg.append("ȫ���ɽ�,");
			break;
		case THOST_FTDC_OST_PartTradedQueueing:
			///���ֳɽ����ڶ�����
			OrderMsg.append("���ֳɽ�(��),");
			break;
		case THOST_FTDC_OST_PartTradedNotQueueing:
			///���ֳɽ����ڶ�����
			OrderMsg.append("���ֳɽ�(�Ƕ�),");
			break;
		case THOST_FTDC_OST_NoTradeQueueing:
			///δ�ɽ����ڶ�����
			OrderMsg.append("δ�ɽ�(��),");
			break;
		case THOST_FTDC_OST_NoTradeNotQueueing:
			///δ�ɽ����ڶ�����
			OrderMsg.append("δ�ɽ�(�Ƕ�),");
			break;
		case THOST_FTDC_OST_Canceled:
			///����
			OrderMsg.append("�ѳ���,");
			break;
		case THOST_FTDC_OST_Unknown:
			OrderMsg.append("δ֪,");
			break;
			///��δ����
		case THOST_FTDC_OST_NotTouched:
			OrderMsg.append("��δ����,");
			break;
			///�Ѵ���
		case THOST_FTDC_OST_Touched:
			OrderMsg.append("�Ѵ���,");
			break;
		default:
			OrderMsg.append(&OrderPtr->OrderStatus);
			OrderMsg.append("_default,");
			break;
		}
		OrderMsg.append(pOrder->StatusMsg);
		m_TradeLog.AddLog(cwTradeLog::enRO, OrderMsg.c_str(), bIsLast);
#endif // TRADELOG
	}

	if (m_CurrentStatus == TradeServerStatus::Status_Initial && bIsLast)
	{
#ifdef CWRISK
		mt.lock();
		for (auto it = m_iCancelLimitMap.begin();
			it != m_iCancelLimitMap.end(); it++)
		{
			std::string OrderMsg = it->first;
			OrderMsg.append(",,,,,,,,,,,,,CancelCount:");
			OrderMsg.append(std::to_string(it->second));
			m_TradeLog.AddLog(cwTradeLog::enMsg, OrderMsg.c_str());
		}
		mt.unlock();
#endif
		CThostFtdcQryTradeField cwQryFild;
#ifdef _MSC_VER
		ZeroMemory(&cwQryFild, sizeof(CThostFtdcQryTradeField));
		strncpy_s(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
		strncpy_s(cwQryFild.InvestorID, m_strInvestorID.c_str(), m_strInvestorID.size());
#else
		memset(&cwQryFild, 0, sizeof(CThostFtdcQryTradeField));
		strncpy(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
		strncpy(cwQryFild.InvestorID, m_strInvestorID.c_str(), m_strInvestorID.size());
#endif // _MSC_VER

		cwSleep(1000);

		MyReqFunction(cwReqQryTrade, (void *)(&cwQryFild));
	}
}

void cwFtdTradeSpiDemo::OnRspQryTrade(CThostFtdcTradeField * pTrade, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	m_bHasTradesChanged = true;

	if (pTrade != NULL)
	{
		cwTradePtr TradePtr = GetcwTradePtr(pTrade);
		if (TradePtr.get() == NULL)
		{
#ifdef CWCOUTINFO
			m_cwShow.AddLog("OnRspQryTrade: malloc Trade Memory Failed !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
			//std::cout << "OnRspQryTrade: malloc Trade Memory Failed !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
#endif // CWCOUTINFO

			return;
		}

#if 0
		{
#ifdef WIN32
			memcpy_s(TradePtr->BrokerID, sizeof(TradePtr->BrokerID), pTrade->BrokerID, sizeof(pTrade->BrokerID));
			memcpy_s(TradePtr->InvestorID, sizeof(TradePtr->InvestorID), pTrade->InvestorID, sizeof(pTrade->InvestorID));
			memcpy_s(TradePtr->InstrumentID, sizeof(TradePtr->InstrumentID), pTrade->InstrumentID, sizeof(pTrade->InstrumentID));
			memcpy_s(TradePtr->OrderRef, sizeof(TradePtr->OrderRef), pTrade->OrderRef, sizeof(pTrade->OrderRef));
			memcpy_s(TradePtr->UserID, sizeof(TradePtr->UserID), pTrade->UserID, sizeof(pTrade->UserID));
			memcpy_s(TradePtr->TradeID, sizeof(TradePtr->TradeID), pTrade->TradeID, sizeof(pTrade->TradeID));
			TradePtr->Direction = GetCtp2CwDirectionType(pTrade->Direction);
			memcpy_s(TradePtr->OrderSysID, sizeof(TradePtr->OrderSysID), pTrade->OrderSysID, sizeof(pTrade->OrderSysID));
			memcpy_s(TradePtr->ClientID, sizeof(TradePtr->ClientID), pTrade->ClientID, sizeof(pTrade->ClientID));
			TradePtr->OffsetFlag = GetCtp2CwOffsetFlagType(pTrade->OffsetFlag);
			TradePtr->HedgeFlag = GetCtp2CwHedgeFlagType(pTrade->HedgeFlag);
			memcpy_s(TradePtr->TradeDate, sizeof(TradePtr->TradeDate), pTrade->TradeDate, sizeof(pTrade->TradeDate));
			memcpy_s(TradePtr->TradeTime, sizeof(TradePtr->TradeTime), pTrade->TradeTime, sizeof(pTrade->TradeTime));
			TradePtr->Price = pTrade->Price;
			TradePtr->Volume = pTrade->Volume;
			TradePtr->TradeType = GetCtp2CwTradeTypetype(pTrade->TradeType);
			TradePtr->TradeSource = GetCtp2CwTradeSourceType(pTrade->TradeSource);
			memcpy_s(TradePtr->TraderID, sizeof(TradePtr->TraderID), pTrade->TraderID, sizeof(pTrade->TraderID));
			memcpy_s(TradePtr->OrderLocalID, sizeof(TradePtr->OrderLocalID), pTrade->OrderLocalID, sizeof(pTrade->OrderLocalID));
#else
			memcpy(TradePtr->BrokerID, pTrade->BrokerID, sizeof(pTrade->BrokerID));
			memcpy(TradePtr->InvestorID, pTrade->InvestorID, sizeof(pTrade->InvestorID));
			memcpy(TradePtr->InstrumentID, pTrade->InstrumentID, sizeof(pTrade->InstrumentID));
			memcpy(TradePtr->OrderRef, pTrade->OrderRef, sizeof(pTrade->OrderRef));
			memcpy(TradePtr->UserID, pTrade->UserID, sizeof(pTrade->UserID));
			memcpy(TradePtr->TradeID, pTrade->TradeID, sizeof(pTrade->TradeID));
			TradePtr->Direction = GetCtp2CwDirectionType(pTrade->Direction);
			memcpy(TradePtr->OrderSysID, pTrade->OrderSysID, sizeof(pTrade->OrderSysID));
			memcpy(TradePtr->ClientID, pTrade->ClientID, sizeof(pTrade->ClientID));
			TradePtr->OffsetFlag = GetCtp2CwOffsetFlagType(pTrade->OffsetFlag);
			TradePtr->HedgeFlag = GetCtp2CwHedgeFlagType(pTrade->HedgeFlag);
			memcpy(TradePtr->TradeDate, pTrade->TradeDate, sizeof(pTrade->TradeDate));
			memcpy(TradePtr->TradeTime, pTrade->TradeTime, sizeof(pTrade->TradeTime));
			TradePtr->Price = pTrade->Price;
			TradePtr->Volume = pTrade->Volume;
			TradePtr->TradeType = GetCtp2CwTradeTypetype(pTrade->TradeType);
			TradePtr->TradeSource = GetCtp2CwTradeSourceType(pTrade->TradeSource);
			memcpy(TradePtr->TraderID, pTrade->TraderID, sizeof(pTrade->TraderID));
			memcpy(TradePtr->OrderLocalID, pTrade->OrderLocalID, sizeof(pTrade->OrderLocalID));
#endif
		}
#endif

		std::string ExchangeTradeID = pTrade->TradeID;
		std::map<std::string, cwTradePtr>::iterator it;
		it = m_TradeMap.find(ExchangeTradeID);
		if (it == m_TradeMap.end())
		{
			m_TradeMap.insert(std::make_pair(ExchangeTradeID, TradePtr));
		}
		else
		{
			it->second = TradePtr;
		}

#ifdef TRADELOG
		std::string OrderMsg;
		OrderMsg = pTrade->InstrumentID;
		OrderMsg.append(",");
		OrderMsg.append(pTrade->OrderRef);
		OrderMsg.append(",");
		OrderMsg.append(pTrade->OrderSysID);
		OrderMsg.append(",");
		OrderMsg.append(pTrade->TradeID);
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pTrade->Price));
		OrderMsg.append(",");
		OrderMsg.append("N/A");
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pTrade->Volume));
		if (pTrade->Direction == THOST_FTDC_D_Buy)
		{
			OrderMsg.append(",B,");
		}
		else
		{
			OrderMsg.append(",S,");
		}
		switch (pTrade->OffsetFlag)
		{
		case THOST_FTDC_OF_Open:
			OrderMsg.append("Open,N/A,");
			break;
			///ƽ��
		case THOST_FTDC_OF_Close:
			OrderMsg.append("Close,N/A,");
			break;
			///ƽ��
		case THOST_FTDC_OF_CloseToday:
			OrderMsg.append("CloseTd,N/A,");
			break;
			///ƽ��
		case THOST_FTDC_OF_CloseYesterday:
			OrderMsg.append("CloseYd,N/A,");
			break;
		default:
			break;
		}
		OrderMsg.append(pTrade->TradeTime);
		OrderMsg.append(",N/A,N/A,N/A");
		m_TradeLog.AddLog(cwTradeLog::enRT, OrderMsg.c_str(), bIsLast);
#endif // TRADELOG

	}

	if (m_CurrentStatus == TradeServerStatus::Status_Initial && bIsLast)
	{
		m_bHasGetTrades = true;

		CThostFtdcSettlementInfoConfirmField confirm;
#ifdef _MSC_VER
		ZeroMemory(&confirm, sizeof(CThostFtdcSettlementInfoConfirmField));
		strncpy_s(confirm.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(confirm.BrokerID));
		strncpy_s(confirm.InvestorID, m_strInvestorID.c_str(), m_strInvestorID.size());
#else
		memset(&confirm, 0, sizeof(CThostFtdcSettlementInfoConfirmField));
		strncpy(confirm.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(confirm.BrokerID));
		strncpy(confirm.InvestorID, m_strInvestorID.c_str(), m_strInvestorID.size());
#endif // _MSC_VER

		cwSleep(1000);

		MyReqFunction(cwReqSettlementInfoConfirm, (void *)(&confirm));
	}
}

void cwFtdTradeSpiDemo::OnRspSettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField * pSettlementInfoConfirm, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pRspInfo && pRspInfo->ErrorID == 0)
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi:OnRspSettlementInfoConfirm");
		//std::cout << "TradeSpi:OnRspSettlementInfoConfirm" << std::endl;
#endif

	}

	if (bIsLast)
	{
		m_CurrentStatus = TradeServerStatus::Status_Normal;

		if (m_pBasicStrategy != NULL)
		{
			m_pBasicStrategy->_SetReady();
		}

		m_bReqQryThreadRun = true;

		if (!m_MyReqQryThread.joinable())
		{
			std::function<void()> ReqQryFunc = std::bind(&cwFtdTradeSpiDemo::LoopReqQryThread, this);
			m_MyReqQryThread = std::thread(ReqQryFunc);
		}
#ifdef CWCOUTINFO
		m_cwShow.AddLog("Trade Initial Finished! ");
		//std::cout << "Trade Initial Finished! "<< std::endl;
#endif
	}
}


void cwFtdTradeSpiDemo::OnRspQryTradingAccount(CThostFtdcTradingAccountField * pTradingAccount, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pTradingAccount != NULL)
	{
		if (!m_pAccount.get())
		{
			m_pAccount.reset(new cwAccountField());
		}

		m_pAccount->Reset();
#ifdef _MSC_VER
		memcpy_s(m_pAccount->AccountID, sizeof(m_pAccount->AccountID), pTradingAccount->AccountID, sizeof(pTradingAccount->AccountID));
#else
		memcpy(m_pAccount->AccountID, pTradingAccount->AccountID, sizeof(pTradingAccount->AccountID));
#endif // _MSC_VER

		m_pAccount->PreBalance = pTradingAccount->PreBalance;
		m_pAccount->Deposit = pTradingAccount->Deposit;
		m_pAccount->Withdraw = pTradingAccount->Withdraw;
		m_pAccount->CurrMargin = pTradingAccount->CurrMargin;
		m_pAccount->Commission = pTradingAccount->Commission;
		m_pAccount->FrozenMargin = pTradingAccount->FrozenMargin;
		m_pAccount->FrozenCommission = pTradingAccount->FrozenCommission;
		m_pAccount->CloseProfit = pTradingAccount->CloseProfit;
		m_pAccount->PositionProfit = pTradingAccount->PositionProfit;
		m_pAccount->Balance = pTradingAccount->Balance;
		m_pAccount->Available = pTradingAccount->Available;
	}
}

void cwFtdTradeSpiDemo::OnRspOrderInsert(CThostFtdcInputOrderField * pInputOrder, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pInputOrder != NULL)
	{
		m_cwOrderRef.UpdateOrderRef(pInputOrder->OrderRef);

		cwOrderPtr OrderPtr = GetcwOrderPtr(pInputOrder);
		if (OrderPtr.get() == NULL)
		{
#ifdef CWCOUTINFO
			m_cwShow.AddLog("\n %s %s %.3f, %d %s",
				pInputOrder->InstrumentID,
				(pInputOrder->Direction == THOST_FTDC_D_Buy ? "B" : "S"),
				pInputOrder->LimitPrice,
				pInputOrder->VolumeTotalOriginal,
				pRspInfo->ErrorMsg);
			m_cwShow.AddLog("OnRspOrderInsert: malloc Order Memory Failed !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
			//std::cout << std::endl;
			//std::cout << pInputOrder->InstrumentID << " "
			//	<< (pInputOrder->Direction == THOST_FTDC_D_Buy ? "B " : "S ")
			//	<< pInputOrder->LimitPrice << " "
			//	<< pInputOrder->VolumeTotalOriginal << " "
			//	<< pRspInfo->ErrorMsg << std::endl;
			//std::cout << "OnRspOrderInsert: malloc Order Memory Failed !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
#endif // CWCOUTINFO
			return;
		}

#if 0
		{
#ifdef WIN32
			///���͹�˾����
			memcpy_s(OrderPtr->BrokerID, sizeof(OrderPtr->BrokerID), pInputOrder->BrokerID, sizeof(pInputOrder->BrokerID));
			///Ͷ���ߴ���
			memcpy_s(OrderPtr->InvestorID, sizeof(OrderPtr->InvestorID), pInputOrder->InvestorID, sizeof(pInputOrder->InvestorID));
			///��Լ����
			memcpy_s(OrderPtr->InstrumentID, sizeof(OrderPtr->InstrumentID), pInputOrder->InstrumentID, sizeof(pInputOrder->InstrumentID));
			///��������
			memcpy_s(OrderPtr->OrderRef, sizeof(OrderPtr->OrderRef), pInputOrder->OrderRef, sizeof(pInputOrder->OrderRef));
			///�û�����
			memcpy_s(OrderPtr->UserID, sizeof(OrderPtr->UserID), pInputOrder->UserID, sizeof(pInputOrder->UserID));
			///�����۸�����
			OrderPtr->OrderPriceType = GetCtp2CwOrderPriceType(pInputOrder->OrderPriceType);
			///��������
			OrderPtr->Direction = GetCtp2CwDirectionType(pInputOrder->Direction);
			///��Ͽ�ƽ��־
			memcpy_s(OrderPtr->CombOffsetFlag, sizeof(OrderPtr->CombOffsetFlag), pInputOrder->CombOffsetFlag, sizeof(pInputOrder->CombOffsetFlag));
			///���Ͷ���ױ���־
			memcpy_s(OrderPtr->CombHedgeFlag, sizeof(OrderPtr->CombHedgeFlag), pInputOrder->CombHedgeFlag, sizeof(pInputOrder->CombHedgeFlag));
			///�۸�
			OrderPtr->LimitPrice = pInputOrder->LimitPrice;
			///����
			OrderPtr->VolumeTotalOriginal = pInputOrder->VolumeTotalOriginal;
			///��ɽ�����
			OrderPtr->VolumeTraded = 0;
			///ʣ������
			OrderPtr->VolumeTotal = pInputOrder->VolumeTotalOriginal;
			///��Ч������
			OrderPtr->TimeCondition = pInputOrder->TimeCondition;
			///GTD����
			memcpy_s(OrderPtr->GTDDate, sizeof(OrderPtr->GTDDate), pInputOrder->GTDDate, sizeof(pInputOrder->GTDDate));
			///�ɽ�������
			OrderPtr->VolumeCondition = pInputOrder->VolumeCondition;
			///��С�ɽ���
			OrderPtr->MinVolume = pInputOrder->MinVolume;
			///��������
			OrderPtr->ContingentCondition = pInputOrder->ContingentCondition;
			///ֹ���
			OrderPtr->StopPrice = pInputOrder->StopPrice;
			///ǿƽԭ��
			OrderPtr->ForceCloseReason = pInputOrder->ForceCloseReason;
			///�Զ������־
			OrderPtr->IsAutoSuspend = pInputOrder->IsAutoSuspend;
			///ҵ��Ԫ
			memcpy_s(OrderPtr->BusinessUnit, sizeof(OrderPtr->BusinessUnit), pInputOrder->BusinessUnit, sizeof(pInputOrder->BusinessUnit));
			///������
			OrderPtr->RequestID = pInputOrder->RequestID;
#ifdef _MSC_VER
#pragma region NoOrderInputField
#endif
			///���ر������
			//TThostFtdcOrderLocalIDType	OrderLocalID;
			///����������
			//TThostFtdcExchangeIDType	ExchangeID;
			///��Ա����
			//TThostFtdcParticipantIDType	ParticipantID;
			///�ͻ�����
			//TThostFtdcClientIDType	ClientID;
			///��Լ�ڽ������Ĵ���
			//TThostFtdcExchangeInstIDType	ExchangeInstID;
			///����������Ա����
			//TThostFtdcTraderIDType	TraderID;
			///��װ���
			//TThostFtdcInstallIDType	InstallID;
			///�����ύ״̬
			//TThostFtdcOrderSubmitStatusType	OrderSubmitStatus;
			///������ʾ���
			//TThostFtdcSequenceNoType	NotifySequence;
			///������
			//TThostFtdcDateType	TradingDay;
			///������
			//TThostFtdcSettlementIDType	SettlementID;
			///�������
			//TThostFtdcOrderSysIDType	OrderSysID;
			///������Դ
			//TThostFtdcOrderSourceType	OrderSource;
			///����״̬
			//TThostFtdcOrderStatusType	OrderStatus;
			///��������
			//TThostFtdcOrderTypeType	OrderType;
			///��������
			//TThostFtdcDateType	InsertDate;
			///ί��ʱ��
			//TThostFtdcTimeType	InsertTime;
			///����ʱ��
			//TThostFtdcTimeType	ActiveTime;
			///����ʱ��
			//TThostFtdcTimeType	SuspendTime;
			///����޸�ʱ��
			//TThostFtdcTimeType	UpdateTime;
			///����ʱ��
			//TThostFtdcTimeType	CancelTime;
			///����޸Ľ���������Ա����
			//TThostFtdcTraderIDType	ActiveTraderID;
			///�����Ա���
			//TThostFtdcParticipantIDType	ClearingPartID;
			///���
			//TThostFtdcSequenceNoType	SequenceNo;
			///ǰ�ñ��
			//TThostFtdcFrontIDType	FrontID;
			///�Ự���
			//TThostFtdcSessionIDType	SessionID;
			///�û��˲�Ʒ��Ϣ
			//TThostFtdcProductInfoType	UserProductInfo;
			///״̬��Ϣ
			//TThostFtdcErrorMsgType	StatusMsg;
			///�����û�����
			//TThostFtdcUserIDType	ActiveUserID;
			///���͹�˾�������
			//TThostFtdcSequenceNoType	BrokerOrderSeq;
			///��ر���
			//TThostFtdcOrderSysIDType	RelativeOrderSysID;
			///֣�����ɽ�����
			//TThostFtdcVolumeType	ZCETotalTradedVolume;
#ifdef _MSC_VER
#pragma endregion
#endif
	///�û�ǿ����־
			OrderPtr->UserForceClose = pInputOrder->UserForceClose;
			///��������־
			OrderPtr->IsSwapOrder = pInputOrder->IsSwapOrder;
			///Ӫҵ�����
			//TThostFtdcBranchIDType	BranchID;
			///Ͷ�ʵ�Ԫ����
			memcpy_s(OrderPtr->InvestUnitID, sizeof(OrderPtr->InvestUnitID), pInputOrder->InvestUnitID, sizeof(pInputOrder->InvestUnitID));
			///�ʽ��˺�
			memcpy_s(OrderPtr->AccountID, sizeof(OrderPtr->AccountID), pInputOrder->AccountID, sizeof(pInputOrder->AccountID));
			///���ִ���
			memcpy_s(OrderPtr->CurrencyID, sizeof(OrderPtr->CurrencyID), pInputOrder->CurrencyID, sizeof(pInputOrder->CurrencyID));
			///IP��ַ
			memcpy_s(OrderPtr->IPAddress, sizeof(OrderPtr->IPAddress), pInputOrder->IPAddress, sizeof(pInputOrder->IPAddress));
			///Mac��ַ
			memcpy_s(OrderPtr->MacAddress, sizeof(OrderPtr->MacAddress), pInputOrder->MacAddress, sizeof(pInputOrder->MacAddress));
#else
			Reset();
			///���͹�˾����
			memcpy(OrderPtr->BrokerID, pInputOrder->BrokerID, sizeof(pInputOrder->BrokerID));
			///Ͷ���ߴ���
			memcpy(OrderPtr->InvestorID, pInputOrder->InvestorID, sizeof(pInputOrder->InvestorID));
			///��Լ����
			memcpy(OrderPtr->InstrumentID, pInputOrder->InstrumentID, sizeof(pInputOrder->InstrumentID));
			///��������
			memcpy(OrderPtr->OrderRef, pInputOrder->OrderRef, sizeof(pInputOrder->OrderRef));
			///�û�����
			memcpy(OrderPtr->UserID, pInputOrder->UserID, sizeof(pInputOrder->UserID));
			///�����۸�����
			OrderPtr->OrderPriceType = pInputOrder->OrderPriceType;
			///��������
			OrderPtr->Direction = pInputOrder->Direction;
			///��Ͽ�ƽ��־
			memcpy(OrderPtr->CombOffsetFlag, pInputOrder->CombOffsetFlag, sizeof(pInputOrder->CombOffsetFlag));
			///���Ͷ���ױ���־
			memcpy(OrderPtr->CombHedgeFlag, pInputOrder->CombHedgeFlag, sizeof(pInputOrder->CombHedgeFlag));
			///�۸�
			OrderPtr->LimitPrice = pInputOrder->LimitPrice;
			///����
			OrderPtr->VolumeTotalOriginal = pInputOrder->VolumeTotalOriginal;
			///��ɽ�����
			OrderPtr->VolumeTraded = 0;
			///ʣ������
			OrderPtr->VolumeTotal = pInputOrder->VolumeTotalOriginal;
			///��Ч������
			OrderPtr->TimeCondition = pInputOrder->TimeCondition;
			///GTD����
			memcpy(OrderPtr->GTDDate, pInputOrder->GTDDate, sizeof(pInputOrder->GTDDate));
			///�ɽ�������
			OrderPtr->VolumeCondition = pInputOrder->VolumeCondition;
			///��С�ɽ���
			OrderPtr->MinVolume = pInputOrder->MinVolume;
			///��������
			OrderPtr->ContingentCondition = pInputOrder->ContingentCondition;
			///ֹ���
			OrderPtr->StopPrice = pInputOrder->StopPrice;
			///ǿƽԭ��
			OrderPtr->ForceCloseReason = pInputOrder->ForceCloseReason;
			///�Զ������־
			OrderPtr->IsAutoSuspend = pInputOrder->IsAutoSuspend;
			///ҵ��Ԫ
			memcpy(OrderPtr->BusinessUnit, pInputOrder->BusinessUnit, sizeof(pInputOrder->BusinessUnit));
			///������
			OrderPtr->RequestID = pInputOrder->RequestID;
#ifdef _MSC_VER
#pragma region NoOrderInputField
#endif
			///���ر������
			//TThostFtdcOrderLocalIDType	OrderLocalID;
			///����������
			//TThostFtdcExchangeIDType	ExchangeID;
			///��Ա����
			//TThostFtdcParticipantIDType	ParticipantID;
			///�ͻ�����
			//TThostFtdcClientIDType	ClientID;
			///��Լ�ڽ������Ĵ���
			//TThostFtdcExchangeInstIDType	ExchangeInstID;
			///����������Ա����
			//TThostFtdcTraderIDType	TraderID;
			///��װ���
			//TThostFtdcInstallIDType	InstallID;
			///�����ύ״̬
			//TThostFtdcOrderSubmitStatusType	OrderSubmitStatus;
			///������ʾ���
			//TThostFtdcSequenceNoType	NotifySequence;
			///������
			//TThostFtdcDateType	TradingDay;
			///������
			//TThostFtdcSettlementIDType	SettlementID;
			///�������
			//TThostFtdcOrderSysIDType	OrderSysID;
			///������Դ
			//TThostFtdcOrderSourceType	OrderSource;
			///����״̬
			//TThostFtdcOrderStatusType	OrderStatus;
			///��������
			//TThostFtdcOrderTypeType	OrderType;
			///��������
			//TThostFtdcDateType	InsertDate;
			///ί��ʱ��
			//TThostFtdcTimeType	InsertTime;
			///����ʱ��
			//TThostFtdcTimeType	ActiveTime;
			///����ʱ��
			//TThostFtdcTimeType	SuspendTime;
			///����޸�ʱ��
			//TThostFtdcTimeType	UpdateTime;
			///����ʱ��
			//TThostFtdcTimeType	CancelTime;
			///����޸Ľ���������Ա����
			//TThostFtdcTraderIDType	ActiveTraderID;
			///�����Ա���
			//TThostFtdcParticipantIDType	ClearingPartID;
			///���
			//TThostFtdcSequenceNoType	SequenceNo;
			///ǰ�ñ��
			//TThostFtdcFrontIDType	FrontID;
			///�Ự���
			//TThostFtdcSessionIDType	SessionID;
			///�û��˲�Ʒ��Ϣ
			//TThostFtdcProductInfoType	UserProductInfo;
			///״̬��Ϣ
			//TThostFtdcErrorMsgType	StatusMsg;
			///�����û�����
			//TThostFtdcUserIDType	ActiveUserID;
			///���͹�˾�������
			//TThostFtdcSequenceNoType	BrokerOrderSeq;
			///��ر���
			//TThostFtdcOrderSysIDType	RelativeOrderSysID;
			///֣�����ɽ�����
			//TThostFtdcVolumeType	ZCETotalTradedVolume;
#ifdef _MSC_VER
#pragma endregion
#endif
///�û�ǿ����־
			OrderPtr->UserForceClose = pInputOrder->UserForceClose;
			///��������־
			OrderPtr->IsSwapOrder = pInputOrder->IsSwapOrder;
			///Ӫҵ�����
			//TThostFtdcBranchIDType	BranchID;
			///Ͷ�ʵ�Ԫ����
			memcpy(OrderPtr->InvestUnitID, pInputOrder->InvestUnitID, sizeof(pInputOrder->InvestUnitID));
			///�ʽ��˺�
			memcpy(OrderPtr->AccountID, pInputOrder->AccountID, sizeof(pInputOrder->AccountID));
			///���ִ���
			memcpy(OrderPtr->CurrencyID, pInputOrder->CurrencyID, sizeof(pInputOrder->CurrencyID));
			///IP��ַ
			memcpy(OrderPtr->IPAddress, pInputOrder->IPAddress, sizeof(pInputOrder->IPAddress));
			///Mac��ַ
			memcpy(OrderPtr->MacAddress, pInputOrder->MacAddress, sizeof(pInputOrder->MacAddress));
#endif
		}
#endif

		m_bHasActiveOrdersChanged = true;
		std::string LocalOrderID = pInputOrder->OrderRef;

		std::map<std::string, cwOrderPtr>::iterator it;
		cwAUTOMUTEX mt(m_TradeSpiMutex, true);
		it = m_ActiveOrdersMap.find(LocalOrderID);
		if (pRspInfo!= NULL
			&& pRspInfo->ErrorID == 0)
		{
			if (it == m_ActiveOrdersMap.end())
			{
				m_ActiveOrdersMap.insert(std::make_pair(LocalOrderID, OrderPtr));
			}
			else
			{
				OrderPtr->UserCancelStatus = it->second->UserCancelStatus;
				OrderPtr->UserCancelTime = it->second->UserCancelTime;

				it->second = OrderPtr;
			}
			mt.unlock();
		}
		else
		{
			if (it != m_ActiveOrdersMap.end())
			{
				m_ActiveOrdersMap.erase(it);
			}

#ifdef CWRISK
			if (!IsIOCTypeOrder(OrderPtr))
			{
				auto CancelLimitMapIt = m_iCancelLimitMap.find(pInputOrder->InstrumentID);
				if (CancelLimitMapIt != m_iCancelLimitMap.end())
				{
					auto SetIt = m_MayCancelOrderRefSetMap.find(pInputOrder->InstrumentID);
					if (SetIt != m_MayCancelOrderRefSetMap.end()
						&& SetIt->second.find(OrderPtr->OrderRef) != SetIt->second.end())
					{
						CancelLimitMapIt->second--;
					}
				}
			}
#endif // CWRISK

			mt.unlock();
#ifdef TRADELOG
			std::string OrderMsg;
			OrderMsg = pInputOrder->InstrumentID;
			OrderMsg.append(",");
			OrderMsg.append(pInputOrder->OrderRef);
			OrderMsg.append(",,,");
			OrderMsg.append(std::to_string(pInputOrder->LimitPrice));
			OrderMsg.append(",");
			OrderMsg.append(std::to_string(pInputOrder->VolumeTotalOriginal));
			if (pInputOrder->Direction == THOST_FTDC_D_Buy)
			{
				OrderMsg.append(",0,B,");
			}
			else
			{
				OrderMsg.append(",0,S,");
			}
			switch (pInputOrder->CombOffsetFlag[0])
			{
			case THOST_FTDC_OF_Open:
				OrderMsg.append("Open,");
				break;
				///ƽ��
			case THOST_FTDC_OF_Close:
				OrderMsg.append("Close,");
				break;
				///ƽ��
			case THOST_FTDC_OF_CloseToday:
				OrderMsg.append("CloseTd,");
				break;
				///ƽ��
			case THOST_FTDC_OF_CloseYesterday:
				OrderMsg.append("CloseYd,");
				break;
			default:
				break;
			}
			OrderMsg.append(GetInsertOrderTypeString(GetInsertOrderType(OrderPtr)));
			OrderMsg.append(",N/A,N/A,����,");
			OrderMsg.append(pRspInfo->ErrorMsg);
			m_TradeLog.AddLog(cwTradeLog::enErr, OrderMsg.c_str());
#endif // TRADELOG
#ifdef CWCOUTINFO
			m_cwShow.AddLog("\n %s %s %.3f, %d %s",
				pInputOrder->InstrumentID,
				(pInputOrder->Direction == THOST_FTDC_D_Buy ? "B" : "S"),
				pInputOrder->LimitPrice,
				pInputOrder->VolumeTotalOriginal,
				pRspInfo->ErrorMsg);
			//std::cout << std::endl;
			//std::cout << pInputOrder->InstrumentID << " "
			//	<< (pInputOrder->Direction == THOST_FTDC_D_Buy ? "B " : "S ")
			//	<< pInputOrder->LimitPrice << " "
			//	<< pInputOrder->VolumeTotalOriginal << " "
			//	<< pRspInfo->ErrorMsg << std::endl;
#endif // CWCOUTINFO
		}
		
		if (m_pBasicStrategy != NULL)
		{
			cwRspInfoPtr pcwRspInfo(new cwFtdcRspInfoField());
			pcwRspInfo->ErrorID = pRspInfo->ErrorID;
#ifdef _MSC_VER
			memcpy_s(pcwRspInfo->ErrorMsg, sizeof(pcwRspInfo->ErrorMsg), pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg));
#else
			memcpy(pcwRspInfo->ErrorMsg, pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg));
#endif // _MSC_VER

			OrderPtr = GetcwOrderPtr(pInputOrder);
					
			m_pBasicStrategy->_OnRspOrderInsert(OrderPtr, pcwRspInfo);
		}
	}
}

void cwFtdTradeSpiDemo::OnErrRtnOrderInsert(CThostFtdcInputOrderField * pInputOrder, CThostFtdcRspInfoField * pRspInfo)
{
#ifdef TRADELOG
	std::string OrderMsg;
	OrderMsg = pInputOrder->InstrumentID;
	OrderMsg.append(",");
	OrderMsg.append(pInputOrder->OrderRef);
	OrderMsg.append(",,,");
	OrderMsg.append(std::to_string(pInputOrder->LimitPrice));
	OrderMsg.append(",");
	OrderMsg.append(std::to_string(pInputOrder->VolumeTotalOriginal));
	if (pInputOrder->Direction == THOST_FTDC_D_Buy)
	{
		OrderMsg.append(",0,B,");
	}
	else
	{
		OrderMsg.append(",0,S,");
	}
	switch (pInputOrder->CombOffsetFlag[0])
	{
	case THOST_FTDC_OF_Open:
		OrderMsg.append("Open,");
		break;
		///ƽ��
	case THOST_FTDC_OF_Close:
		OrderMsg.append("Close,");
		break;
		///ƽ��
	case THOST_FTDC_OF_CloseToday:
		OrderMsg.append("CloseTd,");
		break;
		///ƽ��
	case THOST_FTDC_OF_CloseYesterday:
		OrderMsg.append("CloseYd,");
		break;
	default:
		break;
	}
	OrderMsg.append(GetInsertOrderTypeString(GetInsertOrderType(pInputOrder->OrderPriceType,
		pInputOrder->ContingentCondition, pInputOrder->TimeCondition, pInputOrder->VolumeCondition)));
	OrderMsg.append(",N/A,N/A,����,");
	OrderMsg.append(pRspInfo->ErrorMsg);
	m_TradeLog.AddLog(cwTradeLog::enErr, OrderMsg.c_str());
#endif // TRADELOG

#ifdef CWCOUTINFO
	m_cwShow.AddLog("\n OnErrRtnOrderInsert : %s, %.3f, %d, %d, %s \n",
		pInputOrder->InstrumentID,
		pInputOrder->LimitPrice,
		pInputOrder->VolumeTotalOriginal,
		pRspInfo->ErrorID,
		pRspInfo->ErrorMsg);
	//std::cout << std::endl;
	//std::cout << "OnErrRtnOrderInsert :" << pInputOrder->InstrumentID << " "
	//	<< pInputOrder->LimitPrice << " "
	//	<< pInputOrder->VolumeTotalOriginal << " "
	//	<< pRspInfo->ErrorID << " " << pRspInfo ->ErrorMsg << std::endl;
	//std::cout << std::endl;
#endif
}



void cwFtdTradeSpiDemo::OnRspOrderAction(CThostFtdcInputOrderActionField * pInputOrderAction, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
	if (pInputOrderAction != NULL)
	{
		cwOrderPtr OrderPtr;
		OrderPtr.reset();

		std::map<std::string, cwOrderPtr>::iterator it;
		std::string ExchangeOrderID = pInputOrderAction->OrderSysID;
		
		cwAUTOMUTEX mt(m_TradeSpiMutex, true);
		it = m_OrdersMap.find(ExchangeOrderID);
		if (it == m_OrdersMap.end())
		{
			/*
			<error id = "ORDER_NOT_FOUND" value = "25" prompt = "CTP:�����Ҳ�����Ӧ����" / >
			<error id = "INSUITABLE_ORDER_STATUS" value = "26" prompt = "CTP:������ȫ�ɽ����ѳ����������ٳ�" / >
			*/
			ExchangeOrderID = pInputOrderAction->OrderRef;
			it = m_ActiveOrdersMap.find(ExchangeOrderID);
			if (it != m_ActiveOrdersMap.end())
			{
				OrderPtr = it->second;

				//if (pRspInfo != NULL
				//	&& (it->second->Order.OrderStatus == THOST_FTDC_OST_NoTradeQueueing || it->second->Order.OrderStatus == THOST_FTDC_OST_PartTradedQueueing)
				//	&& (pRspInfo->ErrorID == 25 || pRspInfo->ErrorID == 26))
				//{
				//	m_ActiveOrdersMap.erase(it);
				//}
			}
		}
		else
		{
			OrderPtr = it->second;

			//if (pRspInfo != NULL
			//	&& (it->second->Order.OrderStatus == THOST_FTDC_OST_NoTradeQueueing || it->second->Order.OrderStatus == THOST_FTDC_OST_PartTradedQueueing)
			//	&& (pRspInfo->ErrorID == 25 || pRspInfo->ErrorID == 26))
			//{
			//	m_ActiveOrderMutex.lock();
			//	m_ActiveOrdersMap.erase(pInputOrderAction->OrderRef);
			//	m_ActiveOrderMutex.unlock();
			//}
		}
		mt.unlock();

		cwFtdcRspInfoField RspInfo;
		RspInfo.ErrorID = pRspInfo->ErrorID;
#ifdef _MSC_VER
		memcpy_s(RspInfo.ErrorMsg, sizeof(RspInfo.ErrorMsg), pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg));
#else
		memcpy(RspInfo.ErrorMsg, pRspInfo->ErrorMsg, sizeof(pRspInfo->ErrorMsg));
#endif // _MSC_VER

		if (pRspInfo != NULL
			&& pRspInfo->ErrorID != 0)
		{
#ifdef TRADELOG
			std::string OrderMsg;
			OrderMsg = pInputOrderAction->InstrumentID;
			OrderMsg.append(",");
			OrderMsg.append(pInputOrderAction->OrderRef);
			OrderMsg.append(",");
			OrderMsg.append(pInputOrderAction->OrderSysID);
			OrderMsg.append(",,");
			OrderMsg.append(std::to_string(pInputOrderAction->LimitPrice));
			OrderMsg.append(",,,,,,,,,");
			OrderMsg.append(pRspInfo->ErrorMsg);
			m_TradeLog.AddLog(cwTradeLog::enErr, OrderMsg.c_str());
#endif // TRADELOG
		}
	}

}

void cwFtdTradeSpiDemo::OnErrRtnOrderAction(CThostFtdcOrderActionField * pOrderAction, CThostFtdcRspInfoField * pRspInfo)
{
#ifdef CWCOUTINFO
	m_cwShow.AddLog(" \n OnErrRtnOrderAction : %s %d %s \n",
		pOrderAction->InstrumentID,
		pRspInfo->ErrorID,
		pRspInfo->ErrorMsg);
	//std::cout << std::endl;
	//std::cout << "OnErrRtnOrderAction :" << pOrderAction->InstrumentID <<" "<< pRspInfo->ErrorID << " " << pRspInfo->ErrorMsg << std::endl;
	//std::cout << std::endl;
#endif

	if (pRspInfo != NULL
		&& pRspInfo->ErrorID != 0)
	{
#ifdef TRADELOG
		std::string OrderMsg;
		OrderMsg = pOrderAction->InstrumentID;
		OrderMsg.append(",");
		OrderMsg.append(pOrderAction->OrderRef);
		OrderMsg.append(",");
		OrderMsg.append(pOrderAction->OrderSysID);
		OrderMsg.append(",,");
		OrderMsg.append(std::to_string(pOrderAction->LimitPrice));
		OrderMsg.append(",,,,,,,,,");
		OrderMsg.append(pRspInfo->ErrorMsg);
		m_TradeLog.AddLog(cwTradeLog::enErr, OrderMsg.c_str());
#endif // TRADELOG
	}
}

void cwFtdTradeSpiDemo::OnRtnOrder(CThostFtdcOrderField * pOrder)
{
	m_bHasOrdersChanged = true;
	m_bHasActiveOrdersChanged = true;

	if (pOrder != NULL)
	{
		m_cwOrderRef.UpdateOrderRef(pOrder->OrderRef);

		cwOrderPtr OriginOrderPtr;
		cwOrderPtr OrderPtr = GetcwOrderPtr(pOrder);
		if (OrderPtr.get() == NULL)
		{
#ifdef CWCOUTINFO
			m_cwShow.AddLog("\n %s %s %.3f %d",
				pOrder->InstrumentID,
				(pOrder->Direction == THOST_FTDC_D_Buy ? "B" : "S"),
				pOrder->LimitPrice,
				pOrder->VolumeTotalOriginal);
			m_cwShow.AddLog("OnRtnOrder: malloc Order Memory Failed !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
			//std::cout << std::endl;
			//std::cout << pOrder->InstrumentID << " "
			//	<< (pOrder->Direction == THOST_FTDC_D_Buy ? "B " : "S ")
			//	<< pOrder->LimitPrice << " "
			//	<< pOrder->VolumeTotalOriginal << " "
			//	<< std::endl;
			//std::cout << "OnRtnOrder: malloc Order Memory Failed !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
#endif // CWCOUTINFO
			return;
		}
	
		cwAUTOMUTEX mt(m_TradeSpiMutex, true);
		std::map<std::string, cwOrderPtr>::iterator it;
		std::string ExchangeOrderID = pOrder->OrderSysID;

		if (OrderPtr->CombOffsetFlag[0] == CW_FTDC_OF_Open)
		{
			m_bHasOpenOffsetOrderMap.insert(std::pair<std::string, bool>(OrderPtr->InstrumentID, true));
		}

		if (ExchangeOrderID.size() > 1)
		{
			const std::string InstrumentID = pOrder->InstrumentID;
			std::map<std::string, cwPositionPtr>::iterator Posit;
			Posit = m_PositionMap.find(InstrumentID);
			if (Posit == m_PositionMap.end())
			{
				cwPositionPtr newPos(new cwPositionField);

				m_PositionMap.insert(std::pair<std::string, cwPositionPtr>(InstrumentID, newPos));
				Posit = m_PositionMap.find(InstrumentID);
			}

			it = m_OrdersMap.find(ExchangeOrderID);
			if (it == m_OrdersMap.end())
			{
				Posit->second->UpdatePosition(cwOrderPtr(), OrderPtr);

				m_OrdersMap.insert(std::make_pair(ExchangeOrderID, OrderPtr));
#ifdef CWRISK
				if (!IsIOCTypeOrder(OrderPtr))
				{
					auto CancelLimitMapIt = m_iCancelLimitMap.find(InstrumentID);
					if (CancelLimitMapIt == m_iCancelLimitMap.end())
					{
						m_iCancelLimitMap.insert(std::pair<std::string, int>(InstrumentID, 1));
					}
					else
					{
						auto SetIt = m_MayCancelOrderRefSetMap.find(InstrumentID);
						if (SetIt == m_MayCancelOrderRefSetMap.end()
							|| SetIt->second.find(OrderPtr->OrderRef) == SetIt->second.end())
						{
							CancelLimitMapIt->second++;
						}
					}
				}
#endif // CWRISK
			}
			else
			{
				OriginOrderPtr = it->second;

				Posit->second->UpdatePosition(it->second, OrderPtr);

				OrderPtr->UserCancelStatus = it->second->UserCancelStatus;
				OrderPtr->UserCancelTime = it->second->UserCancelTime;

				it->second = OrderPtr;
#ifdef CWRISK
				if (!IsIOCTypeOrder(OrderPtr))
				{
					switch (pOrder->OrderStatus)
					{
					case THOST_FTDC_OST_AllTraded:
					{
						auto CancelLimitMapIt = m_iCancelLimitMap.find(InstrumentID);
						if (CancelLimitMapIt == m_iCancelLimitMap.end())
						{
							m_iCancelLimitMap.insert(std::pair<std::string, int>(InstrumentID, 0));
						}
						else
						{
							if (CancelLimitMapIt->second > 0)
							{
								CancelLimitMapIt->second--;
							}
						}
					}
					break;
					case THOST_FTDC_OST_PartTradedNotQueueing:
						///���ֳɽ����ڶ�����
					case THOST_FTDC_OST_NoTradeNotQueueing:
						///δ�ɽ����ڶ�����
					case THOST_FTDC_OST_PartTradedQueueing:
						///���ֳɽ����ڶ�����
					case THOST_FTDC_OST_NoTradeQueueing:
						///δ�ɽ����ڶ�����
					case THOST_FTDC_OST_Canceled:
						///����
					default:
						break;
					}
				}
#endif // CWRISK
			}
		}
		else
		{
			switch (pOrder->OrderStatus)
			{
			case THOST_FTDC_OST_AllTraded:
				//ȫ���ɽ�
			case THOST_FTDC_OST_PartTradedNotQueueing:
				///���ֳɽ����ڶ�����
			case THOST_FTDC_OST_NoTradeNotQueueing:
				///δ�ɽ����ڶ�����
			case THOST_FTDC_OST_Canceled:
				///����
#ifdef CWRISK
			if (!IsIOCTypeOrder(OrderPtr))
			{
				auto CancelLimitMapIt = m_iCancelLimitMap.find(pOrder->InstrumentID);
				if (CancelLimitMapIt != m_iCancelLimitMap.end())
				{
					auto SetIt = m_MayCancelOrderRefSetMap.find(pOrder->InstrumentID);
					if (SetIt != m_MayCancelOrderRefSetMap.end()
						&& SetIt->second.find(OrderPtr->OrderRef) != SetIt->second.end())
					{
						CancelLimitMapIt->second--;
					}
				}
			}
#endif // CWRISK
				break;
			case THOST_FTDC_OST_PartTradedQueueing:
				///���ֳɽ����ڶ�����
			case THOST_FTDC_OST_NoTradeQueueing:
				///δ�ɽ����ڶ�����
			case THOST_FTDC_OST_Unknown:
				///δ֪
				break;
			default:
				break;
			}
		}

		//Insert Active Order Map
		switch (pOrder->OrderStatus)
		{
		case THOST_FTDC_OST_AllTraded:
			//ȫ���ɽ�
		case THOST_FTDC_OST_PartTradedNotQueueing:
			///���ֳɽ����ڶ�����
		case THOST_FTDC_OST_NoTradeNotQueueing:
			///δ�ɽ����ڶ�����
		case THOST_FTDC_OST_Canceled:
			///����
			m_bHasActiveOrdersChanged = true;
			ExchangeOrderID = pOrder->OrderRef;

			it = m_ActiveOrdersMap.find(ExchangeOrderID);
			if (it != m_ActiveOrdersMap.end())
			{
				m_ActiveOrdersMap.erase(it);
			}
			break;
		case THOST_FTDC_OST_PartTradedQueueing:
			///���ֳɽ����ڶ�����
		case THOST_FTDC_OST_NoTradeQueueing:
			///δ�ɽ����ڶ�����
		case THOST_FTDC_OST_Unknown:
			///δ֪
			m_bHasActiveOrdersChanged = true;
			ExchangeOrderID = pOrder->OrderRef;

			it = m_ActiveOrdersMap.find(ExchangeOrderID);
			if (it == m_ActiveOrdersMap.end())
			{
				m_ActiveOrdersMap.insert(std::make_pair(ExchangeOrderID, OrderPtr));
			}
			else
			{
				OrderPtr->UserCancelStatus = it->second->UserCancelStatus;
				OrderPtr->UserCancelTime = it->second->UserCancelTime;

				it->second = OrderPtr;
			}
			break;
		default:
			break;
		}
		mt.unlock();

#ifdef TRADELOG
		std::string OrderMsg;
		OrderMsg = pOrder->InstrumentID;
		OrderMsg.append(",");
		OrderMsg.append(pOrder->OrderRef);
		OrderMsg.append(",");
		OrderMsg.append(pOrder->OrderSysID);
		OrderMsg.append(",");
		OrderMsg.append(pOrder->UserProductInfo);
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pOrder->LimitPrice));
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pOrder->VolumeTotalOriginal));
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pOrder->VolumeTraded));
		if (pOrder->Direction == THOST_FTDC_D_Buy)
		{
			OrderMsg.append(",B,");
		}
		else
		{
			OrderMsg.append(",S,");
		}
		switch (pOrder->CombOffsetFlag[0])
		{
		case THOST_FTDC_OF_Open:
			OrderMsg.append("Open,");
			break;
			///ƽ��
		case THOST_FTDC_OF_Close:
			OrderMsg.append("Close,");
			break;
			///ƽ��
		case THOST_FTDC_OF_CloseToday:
			OrderMsg.append("CloseTd,");
			break;
			///ƽ��
		case THOST_FTDC_OF_CloseYesterday:
			OrderMsg.append("CloseYd,");
			break;
		default:
			OrderMsg.append(",");
			break;
		}
		OrderMsg.append(GetInsertOrderTypeString(GetInsertOrderType(OrderPtr)));
		OrderMsg.append(",");
		OrderMsg.append(pOrder->InsertTime);
		OrderMsg.append(",");
		OrderMsg.append(pOrder->CancelTime);
		OrderMsg.append(",");
		switch (pOrder->OrderStatus)
		{
		case THOST_FTDC_OST_AllTraded:
			OrderMsg.append("ȫ���ɽ�,");
			break;
		case THOST_FTDC_OST_PartTradedQueueing:
			///���ֳɽ����ڶ�����
			OrderMsg.append("���ֳɽ�(��),");
			break;
		case THOST_FTDC_OST_PartTradedNotQueueing:
			///���ֳɽ����ڶ�����
			OrderMsg.append("���ֳɽ�(�Ƕ�),");
			break;
		case THOST_FTDC_OST_NoTradeQueueing:
			///δ�ɽ����ڶ�����
			OrderMsg.append("δ�ɽ�(��),");
			break;
		case THOST_FTDC_OST_NoTradeNotQueueing:
			///δ�ɽ����ڶ�����
			OrderMsg.append("δ�ɽ�(�Ƕ�),");
			break;
		case THOST_FTDC_OST_Canceled:
			///����
			OrderMsg.append("�ѳ���,");
			break;
		case THOST_FTDC_OST_Unknown:
			OrderMsg.append("δ֪,");
			break;
			///��δ����
		case THOST_FTDC_OST_NotTouched:
			OrderMsg.append("��δ����,");
			break;
			///�Ѵ���
		case THOST_FTDC_OST_Touched:
			OrderMsg.append("�Ѵ���,");
			break;
		default:
			OrderMsg.append(&pOrder->OrderStatus);
			OrderMsg.append("_default,");
			break;
		}
		OrderMsg.append(pOrder->StatusMsg);
		m_TradeLog.AddLog(cwTradeLog::enUO, OrderMsg.c_str(), true);
#endif // TRADELOG

		if (m_pBasicStrategy != NULL
			&& OrderPtr.get() != NULL)
		{
			m_pBasicStrategy->_OnRtnOrder(OrderPtr, OriginOrderPtr);
		}

		if (m_pBasicStrategy != NULL
			&& OrderPtr.get() != NULL)
		{
			switch (OrderPtr->OrderStatus)
			{
			case CW_FTDC_OST_PartTradedNotQueueing:
				///���ֳɽ����ڶ�����
			case CW_FTDC_OST_NoTradeNotQueueing:
				///δ�ɽ����ڶ�����
			case CW_FTDC_OST_Canceled:
				///����
				m_pBasicStrategy->_OnOrderCanceled(OrderPtr);
				break;
			case CW_FTDC_OST_PartTradedQueueing:
				///���ֳɽ����ڶ�����
			case CW_FTDC_OST_NoTradeQueueing:
				///δ�ɽ����ڶ�����
			case CW_FTDC_OST_Unknown:
				///δ֪
				break;
			default:
				break;
			}
		}
	}
}

void cwFtdTradeSpiDemo::OnRspError(CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
#ifdef CWCOUTINFO
	m_cwShow.AddLog("\n OnRspError: %d %s", pRspInfo->ErrorID, pRspInfo->ErrorMsg);
	//std::cout << std::endl;
	//std::cout << "OnRspError :" << pRspInfo->ErrorID << " " << pRspInfo->ErrorMsg << std::endl;
	//std::cout << std::endl;
#endif 
}

void cwFtdTradeSpiDemo::OnRtnTrade(CThostFtdcTradeField * pTrade)
{
	AddMyReqToList(cwReqType::cwRspQryInvestorPosition);

	m_bHasTradesChanged = true;

	if (pTrade != NULL)
	{
		cwTradePtr TradePtr = GetcwTradePtr(pTrade);
		if (TradePtr.get() == NULL)
		{
#ifdef CWCOUTINFO
			m_cwShow.AddLog("\n %s %s %.3f %d \n",
				pTrade->InstrumentID,
				(pTrade->Direction == THOST_FTDC_D_Buy ? "B" : "S"),
				pTrade->Price,
				pTrade->Volume);
			m_cwShow.AddLog("OnRtnTrade: malloc Trade Memory Failed !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
			//std::cout << std::endl;
			//std::cout << pTrade->InstrumentID << " "
			//	<< (pTrade->Direction == THOST_FTDC_D_Buy ? "B " : "S ")
			//	<< pTrade->Price << " "
			//	<< pTrade->Volume << " "
			//	<< std::endl;
			//std::cout << "OnRtnTrade: malloc Trade Memory Failed !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl;
#endif // CWCOUTINFO

			return;
		}

		m_bHasPositionChanged = true;
		std::map<std::string, cwPositionPtr>::iterator Posit;
		const std::string InstrumentID = TradePtr->InstrumentID;
		cwAUTOMUTEX mt(m_TradeSpiMutex, true);
		Posit = m_PositionMap.find(InstrumentID);
		if (Posit == m_PositionMap.end())
		{
			cwPositionPtr newPos(new cwPositionField);

			m_PositionMap.insert(std::pair<std::string, cwPositionPtr>(InstrumentID, newPos));
			Posit = m_PositionMap.find(InstrumentID);
		}

		auto InsIt = m_InstrumentMap.find(InstrumentID);
		if (InsIt != m_InstrumentMap.end())
		{
			Posit->second->UpdatePosition(TradePtr, InsIt->second);
		}
		else
		{
			cwInstrumentDataPtr InsPtr;
			InsPtr.reset();
			Posit->second->UpdatePosition(TradePtr, InsPtr);
		}
		mt.unlock();

		std::string ExchangeTradeID = pTrade->TradeID;
		std::map<std::string, cwTradePtr>::iterator it;
		it = m_TradeMap.find(ExchangeTradeID);
		if (it == m_TradeMap.end())
		{
			m_TradeMap.insert(std::make_pair(ExchangeTradeID, TradePtr));
		}
		else
		{
			it->second = TradePtr;
		}

#ifdef TRADELOG
		std::string OrderMsg;
		OrderMsg = pTrade->InstrumentID;
		OrderMsg.append(",");
		OrderMsg.append(pTrade->OrderRef);
		OrderMsg.append(",");
		OrderMsg.append(pTrade->OrderSysID);
		OrderMsg.append(",");
		OrderMsg.append(pTrade->TradeID);
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pTrade->Price));
		OrderMsg.append(",N/A,");
		OrderMsg.append(std::to_string(pTrade->Volume));
		if (pTrade->Direction == THOST_FTDC_D_Buy)
		{
			OrderMsg.append(",B,");
		}
		else
		{
			OrderMsg.append(",S,");
		}
		switch (pTrade->OffsetFlag)
		{
		case THOST_FTDC_OF_Open:
			OrderMsg.append("Open,,");
			break;
			///ƽ��
		case THOST_FTDC_OF_Close:
			OrderMsg.append("Close,,");
			break;
			///ƽ��
		case THOST_FTDC_OF_CloseToday:
			OrderMsg.append("CloseTd,,");
			break;
			///ƽ��
		case THOST_FTDC_OF_CloseYesterday:
			OrderMsg.append("CloseYd,,");
			break;
		default:
			OrderMsg.append(",,");
			break;
		}
		OrderMsg.append(pTrade->TradeTime);
		OrderMsg.append(",N/A,N/A,N/A");
		m_TradeLog.AddLog(cwTradeLog::enUT, OrderMsg.c_str(), true);
#endif // TRADELOG


		if (m_pBasicStrategy != NULL)
		{
			m_pBasicStrategy->_OnRtnTrade(TradePtr);
		}
	}
}

cwOrderPtr cwFtdTradeSpiDemo::InputLimitOrder(const char * szInstrumentID, cwFtdcDirectionType direction, cwOpenClose openclose, int volume, double price)
{
	//auto _Starttime = std::chrono::high_resolution_clock::now();

	cwOrderPtr OrderPtr;

	int iCancelCount = 0;
#ifdef CWRISK
	auto CancelLimitMapIt = m_iCancelLimitMap.find(szInstrumentID);
	if (CancelLimitMapIt != m_iCancelLimitMap.end())
	{
		iCancelCount = CancelLimitMapIt->second;
		if (iCancelCount >= m_iMaxCancelLimitNum)
		{
#ifdef TRADELOG
			std::string OrderMsg;
			OrderMsg = szInstrumentID;
			OrderMsg.append(",,,,");
			OrderMsg.append(std::to_string(price));
			OrderMsg.append(",");
			OrderMsg.append(std::to_string(volume));
			OrderMsg.append(",0");
			if (direction == CW_FTDC_D_Buy)
			{
				OrderMsg.append(",B,,");
			}
			else
			{
				OrderMsg.append(",S,,");
			}
			OrderMsg.append(GetInsertOrderTypeString(cwInsertLimitOrder));
			OrderMsg.append(",N/A,N/A,����,CW:�������ޣ���ǰ");
			OrderMsg.append(std::to_string(iCancelCount));
			OrderMsg.append("���޶�");
			OrderMsg.append(std::to_string(m_iMaxCancelLimitNum));

			m_TradeLog.AddLog(cwTradeLog::enIO, OrderMsg.c_str(), true);
#endif // TRADELOG

#ifdef CWCOUTINFO
			m_cwShow.AddLog("%s Cancel Limit!! Current: %d Limit:%d", szInstrumentID, iCancelCount, m_iMaxCancelLimitNum);
			//std::cout << szInstrumentID << " Cancel Limit!! Current:" << iCancelCount << " Limit:" << m_iMaxCancelLimitNum << std::endl;
#endif 
			OrderPtr.reset();
			return OrderPtr;
		}
	}
#endif
	if (m_CurrentStatus != TradeServerStatus::Status_Normal)
	{
#ifdef TRADELOG
		std::string OrderMsg;
		OrderMsg = szInstrumentID;
		OrderMsg.append(",,,,");
		OrderMsg.append(std::to_string(price));
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(volume));
		OrderMsg.append(",0");
		if (direction == CW_FTDC_D_Buy)
		{
			OrderMsg.append(",B,,");
		}
		else
		{
			OrderMsg.append(",S,,");
		}
		OrderMsg.append(GetInsertOrderTypeString(cwInsertLimitOrder));
		OrderMsg.append(",N/A,N/A,����,CW:���׽ӿ���δ����");
		m_TradeLog.AddLog(cwTradeLog::enIO, OrderMsg.c_str(), true);
#endif // TRADELOG

		OrderPtr.reset();
		return OrderPtr;
	}

	auto InsIt = m_InstrumentMap.find(szInstrumentID);
	std::string strExchangeID;
	if (InsIt != m_InstrumentMap.end())
	{
		strExchangeID = InsIt->second->ExchangeID;
	}
	else
	{
#ifdef TRADELOG
		std::string OrderMsg;
		OrderMsg = szInstrumentID;
		OrderMsg.append(",,,,");
		OrderMsg.append(std::to_string(price));
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(volume));
		OrderMsg.append(",0");
		if (direction == CW_FTDC_D_Buy)
		{
			OrderMsg.append(",B,,");
		}
		else
		{
			OrderMsg.append(",S,,");
		}
		OrderMsg.append(GetInsertOrderTypeString(cwInsertLimitOrder));
		OrderMsg.append(",N/A,N/A,����,CW:�޷���֪��Լ��Ϣ");
		m_TradeLog.AddLog(cwTradeLog::enIO, OrderMsg.c_str(), true);
#endif // TRADELOG

		OrderPtr.reset();
		return OrderPtr;
	}

	std::string LocalOrderID = std::to_string(m_cwOrderRef.GetOrderRef());

	CThostFtdcInputOrderField InputOrder;
	memset(&InputOrder, 0, sizeof(InputOrder));

#ifdef _MSC_VER
	strcpy_s(InputOrder.InstrumentID, szInstrumentID);
	strcpy_s(InputOrder.UserID, m_ReqUserLoginField.UserID);
	strcpy_s(InputOrder.InvestorID, m_strInvestorID.c_str());
	strcpy_s(InputOrder.BrokerID, m_ReqUserLoginField.BrokerID);
	strcpy_s(InputOrder.OrderRef, LocalOrderID.c_str());
	strcpy_s(InputOrder.ExchangeID, strExchangeID.c_str());
#else
	strcpy(InputOrder.InstrumentID, szInstrumentID);
	strcpy(InputOrder.UserID, m_ReqUserLoginField.UserID);
	strcpy(InputOrder.InvestorID, m_strInvestorID.c_str());
	strcpy(InputOrder.BrokerID, m_ReqUserLoginField.BrokerID);
	strcpy(InputOrder.OrderRef, LocalOrderID.c_str());
	strcpy(InputOrder.ExchangeID, strExchangeID.c_str());
#endif // _MSC_VER


	InputOrder.Direction = GetCw2CtpDirectionType(direction);
	if (strExchangeID == "SHFE"
		|| strExchangeID == "INE")
	{
		switch (openclose)
		{
		case cwOpenClose::cwOpen:
			InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
			break;
			//������ƽ��ʱ��Ҫ������ֺͽ�֣� 
			//THOST_FTDC_OF_Close�൱��THOST_FTDC_OF_CloseYesterday
		case cwOpenClose::cwClose:
			InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_CloseYesterday;
			break;
		case cwOpenClose::cwCloseToday:
			InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
			break;
		default:
			break;
		}
	}
	else
	{
		switch (openclose)
		{
		case cwOpenClose::cwOpen:
			InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
			break;
			//��������ƽ��ʱ������Ҫ������ֺͽ�֣� ����Ĭ��˳��
			//THOST_FTDC_OF_CloseYesterday��THOST_FTDC_OF_CloseToday�൱��THOST_FTDC_OF_Close
		case cwOpenClose::cwClose:
		case cwOpenClose::cwCloseToday:
			InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
			break;
		default:
			break;
		}
	}
	//strcpy_s(InputOrder.CombOffsetFlag, openclose);
	InputOrder.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;

	InputOrder.VolumeTotalOriginal = volume;
	InputOrder.MinVolume = 1;
	InputOrder.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	InputOrder.IsAutoSuspend = 0;
	InputOrder.UserForceClose = 0;

	//Limit Price Order
	InputOrder.LimitPrice = price;

	InputOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	InputOrder.ContingentCondition = THOST_FTDC_CC_Immediately;
	InputOrder.TimeCondition = THOST_FTDC_TC_GFD;
	InputOrder.VolumeCondition = THOST_FTDC_VC_AV;

	std::map<std::string, cwOrderPtr>::iterator it;

	cwAUTOMUTEX mt(m_TradeSpiMutex, true);
	//����Գɽ�
	strExchangeID = szInstrumentID;
	for (it = m_ActiveOrdersMap.begin();
	it != m_ActiveOrdersMap.end(); it++)
	{
		if (strExchangeID == (std::string)it->second->InstrumentID)
		{
			if (CW_FTDC_D_Buy == direction)
			{
				if (it->second->Direction == CW_FTDC_D_Sell
					&& it->second->LimitPrice <= price + 0.00001)
				{
					//May Self Trading!!!
#ifdef TRADELOG
					std::string OrderMsg;
					OrderMsg = szInstrumentID;
					OrderMsg.append(",");
					OrderMsg.append(InputOrder.OrderRef);
					OrderMsg.append(",,,");
					OrderMsg.append(std::to_string(InputOrder.LimitPrice));
					OrderMsg.append(",");
					OrderMsg.append(std::to_string(InputOrder.VolumeTotalOriginal));
					OrderMsg.append(",0");
					if (InputOrder.Direction == THOST_FTDC_D_Buy)
					{
						OrderMsg.append(",B,");
					}
					else
					{
						OrderMsg.append(",S,");
					}
					switch (InputOrder.CombOffsetFlag[0])
					{
					case THOST_FTDC_OF_Open:
						OrderMsg.append("Open,");
						break;
						///ƽ��
					case THOST_FTDC_OF_Close:
						OrderMsg.append("Close,");
						break;
						///ƽ��
					case THOST_FTDC_OF_CloseToday:
						OrderMsg.append("CloseTd,");
						break;
						///ƽ��
					case THOST_FTDC_OF_CloseYesterday:
						OrderMsg.append("CloseYd,");
						break;
					default:
						break;
					}
					OrderMsg.append(GetInsertOrderTypeString(cwInsertLimitOrder));
					OrderMsg.append(",N/A,N/A,����,CW:���ܳ����Գɽ�");

					m_TradeLog.AddLog(cwTradeLog::enIO, OrderMsg.c_str(), true);
#endif // TRADELOG

					return OrderPtr;
				}
			}
			else
			{
				if (it->second->Direction == CW_FTDC_D_Buy
					&& price <= it->second->LimitPrice + 0.00001)
				{
					//May Self Trading!!!
#ifdef TRADELOG
					std::string OrderMsg;
					OrderMsg = szInstrumentID;
					OrderMsg.append(",");
					OrderMsg.append(InputOrder.OrderRef);
					OrderMsg.append(",,,");
					OrderMsg.append(std::to_string(InputOrder.LimitPrice));
					OrderMsg.append(",");
					OrderMsg.append(std::to_string(InputOrder.VolumeTotalOriginal));
					OrderMsg.append(",0");
					if (InputOrder.Direction == THOST_FTDC_D_Buy)
					{
						OrderMsg.append(",B,");
					}
					else
					{
						OrderMsg.append(",S,");
					}
					switch (InputOrder.CombOffsetFlag[0])
					{
					case THOST_FTDC_OF_Open:
						OrderMsg.append("Open,");
						break;
						///ƽ��
					case THOST_FTDC_OF_Close:
						OrderMsg.append("Close,");
						break;
						///ƽ��
					case THOST_FTDC_OF_CloseToday:
						OrderMsg.append("CloseTd,");
						break;
						///ƽ��
					case THOST_FTDC_OF_CloseYesterday:
						OrderMsg.append("CloseYd,");
						break;
					default:
						break;
					}
					OrderMsg.append(GetInsertOrderTypeString(cwInsertLimitOrder));
					OrderMsg.append(",N/A,N/A,����,CW:���ܳ����Գɽ�");

					m_TradeLog.AddLog(cwTradeLog::enIO, OrderMsg.c_str(), true);
#endif // TRADELOG

					return OrderPtr;

				}
			}
		}
	}

	if (openclose == cwOpenClose::cwOpen)
	{
		m_bHasOpenOffsetOrderMap.insert(std::pair<std::string, bool>(InputOrder.InstrumentID, true));
	}

	m_pTraderUserApi->ReqOrderInsert(&InputOrder, ++m_iRequestId);

	//auto _Endtime = std::chrono::high_resolution_clock::now();
	//m_cwShow.AddLog("pass through Time : %d us",
	//	(_Endtime.time_since_epoch().count() - _Starttime.time_since_epoch().count()) / 1000);

#ifdef CWRISK
	m_MayCancelOrderRefSetMap[InputOrder.InstrumentID].insert(InputOrder.OrderRef);

	{
		auto ret = m_iCancelLimitMap.insert(std::pair<std::string, int>(InputOrder.InstrumentID, 1));
		if (!ret.second)
		{
			ret.first->second++;
		}
	}
#endif

	OrderPtr = GetcwOrderPtr(&InputOrder);
	//OrderPtr.reset(new ORDERFIELD(&InputOrder));

	m_bHasActiveOrdersChanged = true;

	it = m_ActiveOrdersMap.find(LocalOrderID);
	if (it == m_ActiveOrdersMap.end())
	{
		m_ActiveOrdersMap.insert(std::make_pair(LocalOrderID, OrderPtr));
	}
	else
	{
		it->second = OrderPtr;
	}
	mt.unlock();

#ifdef CWCOUTINFO
	m_cwShow.AddLog(" Insert Limit Order %s %s %d %.3f CancelCount: %d",
		szInstrumentID,
		(THOST_FTDC_D_Buy == direction ? "B" : "S"),
		volume,
		price,
		iCancelCount);
	//if (THOST_FTDC_D_Buy == direction)
	//{
	//	std::cout << " Insert Limit Order " << szInstrumentID << " B " << volume << " " << price << " CancelCount:" << iCancelCount << std::endl;
	//}
	//else
	//{
	//	std::cout << " Insert Limit Order " << szInstrumentID << " S " << volume << " " << price << " CancelCount:" << iCancelCount << std::endl;
	//}
#endif 

#ifdef TRADELOG
	std::string OrderMsg;
	OrderMsg = szInstrumentID;
	OrderMsg.append(",");
	OrderMsg.append(InputOrder.OrderRef);
	OrderMsg.append(",,,");
	OrderMsg.append(std::to_string(InputOrder.LimitPrice));
	OrderMsg.append(",");
	OrderMsg.append(std::to_string(InputOrder.VolumeTotalOriginal));
	OrderMsg.append(",0");
	if (InputOrder.Direction == THOST_FTDC_D_Buy)
	{
		OrderMsg.append(",B,");
	}
	else
	{
		OrderMsg.append(",S,");
	}
	switch (InputOrder.CombOffsetFlag[0])
	{
	case THOST_FTDC_OF_Open:
		OrderMsg.append("Open,");
		break;
		///ƽ��
	case THOST_FTDC_OF_Close:
		OrderMsg.append("Close,");
		break;
		///ƽ��
	case THOST_FTDC_OF_CloseToday:
		OrderMsg.append("CloseTd,");
		break;
		///ƽ��
	case THOST_FTDC_OF_CloseYesterday:
		OrderMsg.append("CloseYd,");
		break;
	default:
		break;
	}
	OrderMsg.append(GetInsertOrderTypeString(GetInsertOrderType(OrderPtr)));
	OrderMsg.append(",N/A,N/A,N/A,CancelCount:");
	OrderMsg.append(std::to_string(iCancelCount));

	m_TradeLog.AddLog(cwTradeLog::enIO, OrderMsg.c_str(), true);
#endif // TRADELOG

	return OrderPtr;
}

cwOrderPtr cwFtdTradeSpiDemo::InputFAKOrder(const char * szInstrumentID, cwFtdcDirectionType direction, cwOpenClose openclose, int volume, double price)
{
	cwOrderPtr OrderPtr;

	int iCancelCount = 0;
#ifdef CWRISK
	auto CancelLimitMapIt = m_iCancelLimitMap.find(szInstrumentID);
	if (CancelLimitMapIt != m_iCancelLimitMap.end())
	{
		iCancelCount = CancelLimitMapIt->second;
#if 0
		if (iCancelCount >= m_iMaxCancelLimitNum)
		{
#ifdef CWCOUTINFO
			std::cout << szInstrumentID << " Cancel Limit!! Current:" << iCancelCount << " Limit:" << m_iMaxCancelLimitNum << std::endl;
#endif 
			OrderPtr.reset();
			return OrderPtr;
		}
#endif
	}
#endif
	if (m_CurrentStatus != TradeServerStatus::Status_Normal)
	{
#ifdef TRADELOG
		std::string OrderMsg;
		OrderMsg = szInstrumentID;
		OrderMsg.append(",,,,");
		OrderMsg.append(std::to_string(price));
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(volume));
		OrderMsg.append(",0");
		if (direction == CW_FTDC_D_Buy)
		{
			OrderMsg.append(",B,,");
		}
		else
		{
			OrderMsg.append(",S,,");
		}
		OrderMsg.append(GetInsertOrderTypeString(cwInsertFAKOrder));
		OrderMsg.append(",N/A,N/A,����,CW:���׽ӿ���δ����");

		m_TradeLog.AddLog(cwTradeLog::enIO, OrderMsg.c_str(), true);
#endif // TRADELOG

		OrderPtr.reset();
		return OrderPtr;
	}

	auto InsIt = m_InstrumentMap.find(szInstrumentID);
	std::string strExchangeID;
	if (InsIt != m_InstrumentMap.end())
	{
		strExchangeID = InsIt->second->ExchangeID;
	}
	else
	{
#ifdef TRADELOG
		std::string OrderMsg;
		OrderMsg = szInstrumentID;
		OrderMsg.append(",,,,");
		OrderMsg.append(std::to_string(price));
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(volume));
		OrderMsg.append(",0");
		if (direction == CW_FTDC_D_Buy)
		{
			OrderMsg.append(",B,,");
		}
		else
		{
			OrderMsg.append(",S,,");
		}
		OrderMsg.append(GetInsertOrderTypeString(cwInsertFAKOrder));
		OrderMsg.append(",N/A,N/A,����,CW:�޷���֪��Լ��Ϣ");

		m_TradeLog.AddLog(cwTradeLog::enIO, OrderMsg.c_str(), true);
#endif // TRADELOG

		OrderPtr.reset();
		return OrderPtr;
	}

	std::string LocalOrderID = std::to_string(m_cwOrderRef.GetOrderRef());

	CThostFtdcInputOrderField InputOrder;
	memset(&InputOrder, 0, sizeof(InputOrder));

#ifdef _MSC_VER
	strcpy_s(InputOrder.InstrumentID, szInstrumentID);
	strcpy_s(InputOrder.UserID, m_ReqUserLoginField.UserID);
	strcpy_s(InputOrder.InvestorID, m_strInvestorID.c_str());
	strcpy_s(InputOrder.BrokerID, m_ReqUserLoginField.BrokerID);
	strcpy_s(InputOrder.OrderRef, LocalOrderID.c_str());
	strcpy_s(InputOrder.ExchangeID, strExchangeID.c_str());
#else
	strcpy(InputOrder.InstrumentID, szInstrumentID);
	strcpy(InputOrder.UserID, m_ReqUserLoginField.UserID);
	strcpy(InputOrder.InvestorID, m_strInvestorID.c_str());
	strcpy(InputOrder.BrokerID, m_ReqUserLoginField.BrokerID);
	strcpy(InputOrder.OrderRef, LocalOrderID.c_str());
	strcpy(InputOrder.ExchangeID, strExchangeID.c_str());
#endif // _MSC_VER


	InputOrder.Direction = GetCw2CtpDirectionType(direction);
	if (strExchangeID == "SHFE"
		|| strExchangeID == "INE")
	{
		switch (openclose)
		{
		case cwOpenClose::cwOpen:
			InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
			break;
			//������ƽ��ʱ��Ҫ������ֺͽ�֣� 
			//THOST_FTDC_OF_Close�൱��THOST_FTDC_OF_CloseYesterday
		case cwOpenClose::cwClose:
			InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_CloseYesterday;
			break;
		case cwOpenClose::cwCloseToday:
			InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
			break;
		default:
			break;
}
	}
	else
	{
		switch (openclose)
		{
		case cwOpenClose::cwOpen:
			InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
			break;
			//��������ƽ��ʱ������Ҫ������ֺͽ�֣� ����Ĭ��˳��
			//THOST_FTDC_OF_CloseYesterday��THOST_FTDC_OF_CloseToday�൱��THOST_FTDC_OF_Close
		case cwOpenClose::cwClose:
		case cwOpenClose::cwCloseToday:
			InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
			break;
		default:
			break;
		}
	}
	//strcpy_s(InputOrder.CombOffsetFlag, openclose);
	InputOrder.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;

	InputOrder.VolumeTotalOriginal = volume;
	InputOrder.MinVolume = 1;
	InputOrder.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	InputOrder.IsAutoSuspend = 0;
	InputOrder.UserForceClose = 0;

	//FAK Order
	InputOrder.LimitPrice = price;

	InputOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	InputOrder.ContingentCondition = THOST_FTDC_CC_Immediately;
	InputOrder.TimeCondition = THOST_FTDC_TC_IOC;
	InputOrder.VolumeCondition = THOST_FTDC_VC_AV;

	std::map<std::string, cwOrderPtr>::iterator it;

	cwAUTOMUTEX mt(m_TradeSpiMutex, true);
	//����Գɽ�
	strExchangeID = szInstrumentID;
	for (it = m_ActiveOrdersMap.begin();
	it != m_ActiveOrdersMap.end(); it++)
	{
		if (strExchangeID == (std::string)it->second->InstrumentID)
		{
			if (CW_FTDC_D_Buy == direction)
			{
				if (it->second->Direction == CW_FTDC_D_Sell
					&& it->second->LimitPrice <= price + 0.00001)
				{
					//May Self Trading!!!
#ifdef TRADELOG
					std::string OrderMsg;
					OrderMsg = szInstrumentID;
					OrderMsg.append(",");
					OrderMsg.append(InputOrder.OrderRef);
					OrderMsg.append(",,,");
					OrderMsg.append(std::to_string(InputOrder.LimitPrice));
					OrderMsg.append(",");
					OrderMsg.append(std::to_string(InputOrder.VolumeTotalOriginal));
					OrderMsg.append(",0");
					if (InputOrder.Direction == THOST_FTDC_D_Buy)
					{
						OrderMsg.append(",B,");
					}
					else
					{
						OrderMsg.append(",S,");
					}
					switch (InputOrder.CombOffsetFlag[0])
					{
					case THOST_FTDC_OF_Open:
						OrderMsg.append("Open,");
						break;
						///ƽ��
					case THOST_FTDC_OF_Close:
						OrderMsg.append("Close,");
						break;
						///ƽ��
					case THOST_FTDC_OF_CloseToday:
						OrderMsg.append("CloseTd,");
						break;
						///ƽ��
					case THOST_FTDC_OF_CloseYesterday:
						OrderMsg.append("CloseYd,");
						break;
					default:
						break;
					}
					OrderMsg.append(GetInsertOrderTypeString(cwInsertFAKOrder));
					OrderMsg.append(",N/A,N/A,����,CW:���ܳ����Գɽ�");

					m_TradeLog.AddLog(cwTradeLog::enIO, OrderMsg.c_str(), true);
#endif // TRADELOG

					return OrderPtr;
				}
			}
			else
			{
				if (it->second->Direction == CW_FTDC_D_Buy
					&& price <= it->second->LimitPrice + 0.00001)
				{
					//May Self Trading!!!
#ifdef TRADELOG
					std::string OrderMsg;
					OrderMsg = szInstrumentID;
					OrderMsg.append(",");
					OrderMsg.append(InputOrder.OrderRef);
					OrderMsg.append(",,,");
					OrderMsg.append(std::to_string(InputOrder.LimitPrice));
					OrderMsg.append(",");
					OrderMsg.append(std::to_string(InputOrder.VolumeTotalOriginal));
					OrderMsg.append(",0");
					if (InputOrder.Direction == THOST_FTDC_D_Buy)
					{
						OrderMsg.append(",B,");
					}
					else
					{
						OrderMsg.append(",S,");
					}
					switch (InputOrder.CombOffsetFlag[0])
					{
					case THOST_FTDC_OF_Open:
						OrderMsg.append("Open,");
						break;
						///ƽ��
					case THOST_FTDC_OF_Close:
						OrderMsg.append("Close,");
						break;
						///ƽ��
					case THOST_FTDC_OF_CloseToday:
						OrderMsg.append("CloseTd,");
						break;
						///ƽ��
					case THOST_FTDC_OF_CloseYesterday:
						OrderMsg.append("CloseYd,");
						break;
					default:
						break;
					}
					OrderMsg.append(GetInsertOrderTypeString(cwInsertFAKOrder));
					OrderMsg.append(",N/A,N/A,����,CW:���ܳ����Գɽ�");

					m_TradeLog.AddLog(cwTradeLog::enIO, OrderMsg.c_str(), true);
#endif // TRADELOG

					return OrderPtr;

				}
			}
		}
	}

	if (openclose == cwOpenClose::cwOpen)
	{
		m_bHasOpenOffsetOrderMap.insert(std::pair<std::string, bool>(InputOrder.InstrumentID, true));
	}

	m_pTraderUserApi->ReqOrderInsert(&InputOrder, ++m_iRequestId);
	OrderPtr = GetcwOrderPtr(&InputOrder);
	//OrderPtr.reset(new ORDERFIELD(&InputOrder));

	m_bHasActiveOrdersChanged = true;

	it = m_ActiveOrdersMap.find(LocalOrderID);
	if (it == m_ActiveOrdersMap.end())
	{
		m_ActiveOrdersMap.insert(std::make_pair(LocalOrderID, OrderPtr));
	}
	else
	{
		it->second = OrderPtr;
	}
	mt.unlock();

#ifdef CWCOUTINFO
	m_cwShow.AddLog(" Insert FAK Order %s %s %d %.3f CancelCount:%d",
		szInstrumentID,
		(THOST_FTDC_D_Buy == direction ? "B" : "S"),
		volume,
		price,
		iCancelCount);
	//std::cout << " Insert FAK Order " << szInstrumentID << (THOST_FTDC_D_Buy == direction ? " B " : " S ") << volume << " " << price << " CancelCount:" << iCancelCount << std::endl;
#endif 

#ifdef TRADELOG
	std::string OrderMsg;
	OrderMsg = szInstrumentID;
	OrderMsg.append(",");
	OrderMsg.append(InputOrder.OrderRef);
	OrderMsg.append(",,,");
	OrderMsg.append(std::to_string(InputOrder.LimitPrice));
	OrderMsg.append(",");
	OrderMsg.append(std::to_string(InputOrder.VolumeTotalOriginal));
	OrderMsg.append(",0");
	if (InputOrder.Direction == THOST_FTDC_D_Buy)
	{
		OrderMsg.append(",B,");
	}
	else
	{
		OrderMsg.append(",S,");
	}
	switch (InputOrder.CombOffsetFlag[0])
	{
	case THOST_FTDC_OF_Open:
		OrderMsg.append("Open,");
		break;
		///ƽ��
	case THOST_FTDC_OF_Close:
		OrderMsg.append("Close,");
		break;
		///ƽ��
	case THOST_FTDC_OF_CloseToday:
		OrderMsg.append("CloseTd,");
		break;
		///ƽ��
	case THOST_FTDC_OF_CloseYesterday:
		OrderMsg.append("CloseYd,");
		break;
	default:
		break;
	}
	OrderMsg.append(GetInsertOrderTypeString(GetInsertOrderType(OrderPtr)));
	OrderMsg.append(",N/A,N/A,N/A,CancelCount:");
	OrderMsg.append(std::to_string(iCancelCount));

	m_TradeLog.AddLog(cwTradeLog::enIO, OrderMsg.c_str(), true);
#endif // TRADELOG

	return OrderPtr;
}

cwOrderPtr cwFtdTradeSpiDemo::InputFOKOrder(const char * szInstrumentID, cwFtdcDirectionType direction, cwOpenClose openclose, int volume, double price)
{
	cwOrderPtr OrderPtr;

	int iCancelCount = 0;
#ifdef CWRISK
	auto CancelLimitMapIt = m_iCancelLimitMap.find(szInstrumentID);
	if (CancelLimitMapIt != m_iCancelLimitMap.end())
	{
		iCancelCount = CancelLimitMapIt->second;
#if 0
		if (iCancelCount >= m_iMaxCancelLimitNum)
		{
#ifdef CWCOUTINFO
			std::cout << szInstrumentID << " Cancel Limit!! Current:" << iCancelCount << " Limit:" << m_iMaxCancelLimitNum << std::endl;
#endif 
			OrderPtr.reset();
			return OrderPtr;
		}
#endif
	}
#endif
	if (m_CurrentStatus != TradeServerStatus::Status_Normal)
	{
#ifdef TRADELOG
		std::string OrderMsg;
		OrderMsg = szInstrumentID;
		OrderMsg.append(",,,,");
		OrderMsg.append(std::to_string(price));
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(volume));
		OrderMsg.append(",0");
		if (direction == CW_FTDC_D_Buy)
		{
			OrderMsg.append(",B,,");
		}
		else
		{
			OrderMsg.append(",S,,");
		}
		OrderMsg.append(GetInsertOrderTypeString(cwInsertFOKOrder));
		OrderMsg.append(",N/A,N/A,����,CW:���׽ӿ���δ����");

		m_TradeLog.AddLog(cwTradeLog::enIO, OrderMsg.c_str(), true);
#endif // TRADELOG

		OrderPtr.reset();
		return OrderPtr;
	}

	auto InsIt = m_InstrumentMap.find(szInstrumentID);
	std::string strExchangeID;
	if (InsIt != m_InstrumentMap.end())
	{
		strExchangeID = InsIt->second->ExchangeID;
	}
	else
	{
#ifdef TRADELOG
		std::string OrderMsg;
		OrderMsg = szInstrumentID;
		OrderMsg.append(",,,,");
		OrderMsg.append(std::to_string(price));
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(volume));
		OrderMsg.append(",0");
		if (direction == CW_FTDC_D_Buy)
		{
			OrderMsg.append(",B,,");
		}
		else
		{
			OrderMsg.append(",S,,");
		}
		OrderMsg.append(GetInsertOrderTypeString(cwInsertFOKOrder));
		OrderMsg.append(",N/A,N/A,����,CW:�޷���֪��Լ��Ϣ");

		m_TradeLog.AddLog(cwTradeLog::enIO, OrderMsg.c_str(), true);
#endif // TRADELOG

		OrderPtr.reset();
		return OrderPtr;
	}

	std::string LocalOrderID = std::to_string(m_cwOrderRef.GetOrderRef());

	CThostFtdcInputOrderField InputOrder;
	memset(&InputOrder, 0, sizeof(InputOrder));

#ifdef _MSC_VER
	strcpy_s(InputOrder.InstrumentID, szInstrumentID);
	strcpy_s(InputOrder.UserID, m_ReqUserLoginField.UserID);
	strcpy_s(InputOrder.InvestorID, m_strInvestorID.c_str());
	strcpy_s(InputOrder.BrokerID, m_ReqUserLoginField.BrokerID);
	strcpy_s(InputOrder.OrderRef, LocalOrderID.c_str());
	strcpy_s(InputOrder.ExchangeID, strExchangeID.c_str());
#else
	strcpy(InputOrder.InstrumentID, szInstrumentID);
	strcpy(InputOrder.UserID, m_ReqUserLoginField.UserID);
	strcpy(InputOrder.InvestorID, m_strInvestorID.c_str());
	strcpy(InputOrder.BrokerID, m_ReqUserLoginField.BrokerID);
	strcpy(InputOrder.OrderRef, LocalOrderID.c_str());
	strcpy(InputOrder.ExchangeID, strExchangeID.c_str());
#endif // _MSC_VER


	InputOrder.Direction = GetCw2CtpDirectionType(direction);
	if (strExchangeID == "SHFE"
		|| strExchangeID == "INE")
	{
		switch (openclose)
		{
		case cwOpenClose::cwOpen:
			InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
			break;
			//������ƽ��ʱ��Ҫ������ֺͽ�֣� 
			//THOST_FTDC_OF_Close�൱��THOST_FTDC_OF_CloseYesterday
		case cwOpenClose::cwClose:
			InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_CloseYesterday;
			break;
		case cwOpenClose::cwCloseToday:
			InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
			break;
		default:
			break;
}
	}
	else
	{
		switch (openclose)
		{
		case cwOpenClose::cwOpen:
			InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
			break;
			//��������ƽ��ʱ������Ҫ������ֺͽ�֣� ����Ĭ��˳��
			//THOST_FTDC_OF_CloseYesterday��THOST_FTDC_OF_CloseToday�൱��THOST_FTDC_OF_Close
		case cwOpenClose::cwClose:
		case cwOpenClose::cwCloseToday:
			InputOrder.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
			break;
		default:
			break;
		}
	}
	//strcpy_s(InputOrder.CombOffsetFlag, openclose);
	InputOrder.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;

	InputOrder.VolumeTotalOriginal = volume;
	InputOrder.MinVolume = 1;
	InputOrder.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
	InputOrder.IsAutoSuspend = 0;
	InputOrder.UserForceClose = 0;

	//FOK Order
	InputOrder.LimitPrice = price;

	InputOrder.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
	InputOrder.ContingentCondition = THOST_FTDC_CC_Immediately;
	InputOrder.TimeCondition = THOST_FTDC_TC_IOC;
	InputOrder.VolumeCondition = THOST_FTDC_VC_CV;

	std::map<std::string, cwOrderPtr>::iterator it;

	cwAUTOMUTEX mt(m_TradeSpiMutex, true);
	//����Գɽ�
	strExchangeID = szInstrumentID;
	for (it = m_ActiveOrdersMap.begin();
	it != m_ActiveOrdersMap.end(); it++)
	{
		if (strExchangeID == (std::string)it->second->InstrumentID)
		{
			if (CW_FTDC_D_Buy == direction)
			{
				if (it->second->Direction == CW_FTDC_D_Sell
					&& it->second->LimitPrice <= price + 0.00001)
				{
					//May Self Trading!!!
#ifdef TRADELOG
					std::string OrderMsg;
					OrderMsg = szInstrumentID;
					OrderMsg.append(",");
					OrderMsg.append(InputOrder.OrderRef);
					OrderMsg.append(",,,");
					OrderMsg.append(std::to_string(InputOrder.LimitPrice));
					OrderMsg.append(",");
					OrderMsg.append(std::to_string(InputOrder.VolumeTotalOriginal));
					OrderMsg.append(",0");
					if (InputOrder.Direction == THOST_FTDC_D_Buy)
					{
						OrderMsg.append(",B,");
					}
					else
					{
						OrderMsg.append(",S,");
					}
					switch (InputOrder.CombOffsetFlag[0])
					{
					case THOST_FTDC_OF_Open:
						OrderMsg.append("Open,");
						break;
						///ƽ��
					case THOST_FTDC_OF_Close:
						OrderMsg.append("Close,");
						break;
						///ƽ��
					case THOST_FTDC_OF_CloseToday:
						OrderMsg.append("CloseTd,");
						break;
						///ƽ��
					case THOST_FTDC_OF_CloseYesterday:
						OrderMsg.append("CloseYd,");
						break;
					default:
						break;
					}
					OrderMsg.append(GetInsertOrderTypeString(cwInsertFOKOrder));
					OrderMsg.append(",N/A,N/A,����,CW:���ܳ����Գɽ�");

					m_TradeLog.AddLog(cwTradeLog::enIO, OrderMsg.c_str(), true);
#endif // TRADELOG

					return OrderPtr;
				}
			}
			else
			{
				if (it->second->Direction == CW_FTDC_D_Buy
					&& price <= it->second->LimitPrice + 0.00001)
				{
					//May Self Trading!!!
#ifdef TRADELOG
					std::string OrderMsg;
					OrderMsg = szInstrumentID;
					OrderMsg.append(",");
					OrderMsg.append(InputOrder.OrderRef);
					OrderMsg.append(",,,");
					OrderMsg.append(std::to_string(InputOrder.LimitPrice));
					OrderMsg.append(",");
					OrderMsg.append(std::to_string(InputOrder.VolumeTotalOriginal));
					OrderMsg.append(",0");
					if (InputOrder.Direction == THOST_FTDC_D_Buy)
					{
						OrderMsg.append(",B,");
					}
					else
					{
						OrderMsg.append(",S,");
					}
					switch (InputOrder.CombOffsetFlag[0])
					{
					case THOST_FTDC_OF_Open:
						OrderMsg.append("Open,");
						break;
						///ƽ��
					case THOST_FTDC_OF_Close:
						OrderMsg.append("Close,");
						break;
						///ƽ��
					case THOST_FTDC_OF_CloseToday:
						OrderMsg.append("CloseTd,");
						break;
						///ƽ��
					case THOST_FTDC_OF_CloseYesterday:
						OrderMsg.append("CloseYd,");
						break;
					default:
						break;
					}
					OrderMsg.append(GetInsertOrderTypeString(cwInsertFOKOrder));
					OrderMsg.append(",N/A,N/A,����,CW:���ܳ����Գɽ�");

					m_TradeLog.AddLog(cwTradeLog::enIO, OrderMsg.c_str(), true);
#endif // TRADELOG

					return OrderPtr;

				}
			}
		}
	}

	if (openclose == cwOpenClose::cwOpen)
	{
		m_bHasOpenOffsetOrderMap.insert(std::pair<std::string, bool>(InputOrder.InstrumentID, true));
	}

	m_pTraderUserApi->ReqOrderInsert(&InputOrder, ++m_iRequestId);
	OrderPtr = GetcwOrderPtr(&InputOrder);
	//OrderPtr.reset(new ORDERFIELD(&InputOrder));

	m_bHasActiveOrdersChanged = true;

	it = m_ActiveOrdersMap.find(LocalOrderID);
	if (it == m_ActiveOrdersMap.end())
	{
		m_ActiveOrdersMap.insert(std::make_pair(LocalOrderID, OrderPtr));
	}
	else
	{
		it->second = OrderPtr;
	}
	mt.unlock();

#ifdef CWCOUTINFO
	m_cwShow.AddLog(" Insert FOK Order %s %s %d %.3f CancelCount:%d",
		szInstrumentID,
		(THOST_FTDC_D_Buy == direction ? "B" : "S"),
		volume,
		price,
		iCancelCount);

	//std::cout << " Insert FOK Order " << szInstrumentID << (THOST_FTDC_D_Buy == direction ? " B " : " S ") << volume << " " << price << " CancelCount:" << iCancelCount << std::endl;
#endif 

#ifdef TRADELOG
	std::string OrderMsg;
	OrderMsg = szInstrumentID;
	OrderMsg.append(",");
	OrderMsg.append(InputOrder.OrderRef);
	OrderMsg.append(",,,");
	OrderMsg.append(std::to_string(InputOrder.LimitPrice));
	OrderMsg.append(",");
	OrderMsg.append(std::to_string(InputOrder.VolumeTotalOriginal));
	OrderMsg.append(",0");
	if (InputOrder.Direction == THOST_FTDC_D_Buy)
	{
		OrderMsg.append(",B,");
	}
	else
	{
		OrderMsg.append(",S,");
	}
	switch (InputOrder.CombOffsetFlag[0])
	{
	case THOST_FTDC_OF_Open:
		OrderMsg.append("Open,");
		break;
		///ƽ��
	case THOST_FTDC_OF_Close:
		OrderMsg.append("Close,");
		break;
		///ƽ��
	case THOST_FTDC_OF_CloseToday:
		OrderMsg.append("CloseTd,");
		break;
		///ƽ��
	case THOST_FTDC_OF_CloseYesterday:
		OrderMsg.append("CloseYd,");
		break;
	default:
		break;
	}
	OrderMsg.append(GetInsertOrderTypeString(GetInsertOrderType(OrderPtr)));
	OrderMsg.append(",N/A,N/A,N/A,CancelCount:");
	OrderMsg.append(std::to_string(iCancelCount));

	m_TradeLog.AddLog(cwTradeLog::enIO, OrderMsg.c_str(), true);
#endif // TRADELOG

	return OrderPtr;
}

void cwFtdTradeSpiDemo::CancelOrder(const char * szLocalOrderID)
{
	cwAUTOMUTEX mt(m_TradeSpiMutex, true);
	auto it = m_ActiveOrdersMap.find(szLocalOrderID);
	if (it != m_ActiveOrdersMap.end())
	{
		mt.unlock();
		CancelOrder(it->second);
	}
}

void cwFtdTradeSpiDemo::CancelOrder(cwOrderPtr pOrder)
{
	if (pOrder.get() == NULL)
	{
		return;
	}
	
	if (pOrder->TimeCondition == CW_FTDC_TC_IOC)
	{
#ifdef TRADELOG
		std::string OrderMsg;
		OrderMsg = pOrder->InstrumentID;
		OrderMsg.append(",");
		OrderMsg.append(pOrder->OrderRef);
		OrderMsg.append(",");
		OrderMsg.append(pOrder->OrderSysID);
		OrderMsg.append(",");
		OrderMsg.append(pOrder->UserProductInfo);
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pOrder->LimitPrice));
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pOrder->VolumeTotalOriginal));
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pOrder->VolumeTraded));
		if (pOrder->Direction == CW_FTDC_D_Buy)
		{
			OrderMsg.append(",B,");
		}
		else
		{
			OrderMsg.append(",S,");
		}
		switch (pOrder->CombOffsetFlag[0])
		{
		case CW_FTDC_OF_Open:
			OrderMsg.append("Open,");
			break;
			///ƽ��
		case CW_FTDC_OF_Close:
			OrderMsg.append("Close,");
			break;
			///ƽ��
		case CW_FTDC_OF_CloseToday:
			OrderMsg.append("CloseTd,");
			break;
			///ƽ��
		case CW_FTDC_OF_CloseYesterday:
			OrderMsg.append("CloseYd,");
			break;
		default:
			OrderMsg.append(",");
			break;
		}
		OrderMsg.append(GetInsertOrderTypeString(GetInsertOrderType(pOrder)));
		OrderMsg.append(",");
		OrderMsg.append(pOrder->InsertTime);
		OrderMsg.append(",");
		OrderMsg.append(pOrder->CancelTime);
		OrderMsg.append(",");
		switch (pOrder->OrderStatus)
		{
		case CW_FTDC_OST_AllTraded:
			OrderMsg.append("ȫ���ɽ�,");
			break;
		case CW_FTDC_OST_PartTradedQueueing:
			///���ֳɽ����ڶ�����
			OrderMsg.append("���ֳɽ�(��),");
			break;
		case CW_FTDC_OST_PartTradedNotQueueing:
			///���ֳɽ����ڶ�����
			OrderMsg.append("���ֳɽ�(�Ƕ�),");
			break;
		case CW_FTDC_OST_NoTradeQueueing:
			///δ�ɽ����ڶ�����
			OrderMsg.append("δ�ɽ�(��),");
			break;
		case CW_FTDC_OST_NoTradeNotQueueing:
			///δ�ɽ����ڶ�����
			OrderMsg.append("δ�ɽ�(�Ƕ�),");
			break;
		case CW_FTDC_OST_Canceled:
			///����
			OrderMsg.append("�ѳ���,");
			break;
		case CW_FTDC_OST_AcceptedNoReply:
			///�����ѱ��뽻����δӦ��
			OrderMsg.append("�ѱ�δ��Ӧ,");
			break;
		case CW_FTDC_OST_Unknown:
			OrderMsg.append("δ֪,");
			break;
			///��δ����
		case CW_FTDC_OST_NotTouched:
			OrderMsg.append("��δ����,");
			break;
			///�Ѵ���
		case CW_FTDC_OST_Touched:
			OrderMsg.append("�Ѵ���,");
			break;
		default:
			OrderMsg.append(&pOrder->OrderStatus);
			OrderMsg.append("_default,");
			break;
		}
		OrderMsg.append(pOrder->StatusMsg);
		OrderMsg.append(" ����IOC����������������");
		m_TradeLog.AddLog(cwTradeLog::enCO, OrderMsg.c_str(), true);
#endif // TRADELOG

		return;
	}
	
	if(CW_FTDC_OST_cwDefault == GetCtp2CwOrderStatusType(pOrder->OrderStatus))
	{
#ifdef TRADELOG
		std::string OrderMsg;
		OrderMsg = pOrder->InstrumentID;
		OrderMsg.append(",");
		OrderMsg.append(pOrder->OrderRef);
		OrderMsg.append(",");
		OrderMsg.append(pOrder->OrderSysID);
		OrderMsg.append(",");
		OrderMsg.append(pOrder->UserProductInfo);
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pOrder->LimitPrice));
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pOrder->VolumeTotalOriginal));
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pOrder->VolumeTraded));
		if (pOrder->Direction == CW_FTDC_D_Buy)
		{
			OrderMsg.append(",B,");
		}
		else
		{
			OrderMsg.append(",S,");
		}
		switch (pOrder->CombOffsetFlag[0])
		{
		case CW_FTDC_OF_Open:
			OrderMsg.append("Open,");
			break;
			///ƽ��
		case CW_FTDC_OF_Close:
			OrderMsg.append("Close,");
			break;
			///ƽ��
		case CW_FTDC_OF_CloseToday:
			OrderMsg.append("CloseTd,");
			break;
			///ƽ��
		case CW_FTDC_OF_CloseYesterday:
			OrderMsg.append("CloseYd,");
			break;
		default:
			OrderMsg.append(",");
			break;
		}
		OrderMsg.append(GetInsertOrderTypeString(GetInsertOrderType(pOrder->OrderPriceType,
			pOrder->ContingentCondition, pOrder->TimeCondition, pOrder->VolumeCondition)));
		OrderMsg.append(",");
		OrderMsg.append(pOrder->InsertTime);
		OrderMsg.append(",");
		OrderMsg.append(pOrder->CancelTime);
		OrderMsg.append(",");
		switch (pOrder->OrderStatus)
		{
		case CW_FTDC_OST_AllTraded:
			OrderMsg.append("ȫ���ɽ�,");
			break;
		case CW_FTDC_OST_PartTradedQueueing:
			///���ֳɽ����ڶ�����
			OrderMsg.append("���ֳɽ�(��),");
			break;
		case CW_FTDC_OST_PartTradedNotQueueing:
			///���ֳɽ����ڶ�����
			OrderMsg.append("���ֳɽ�(�Ƕ�),");
			break;
		case CW_FTDC_OST_NoTradeQueueing:
			///δ�ɽ����ڶ�����
			OrderMsg.append("δ�ɽ�(��),");
			break;
		case CW_FTDC_OST_NoTradeNotQueueing:
			///δ�ɽ����ڶ�����
			OrderMsg.append("δ�ɽ�(�Ƕ�),");
			break;
		case CW_FTDC_OST_Canceled:
			///����
			OrderMsg.append("�ѳ���,");
			break;
		case CW_FTDC_OST_AcceptedNoReply:
			///�����ѱ��뽻����δӦ��
			OrderMsg.append("�ѱ�δ��Ӧ,");
			break;
		case CW_FTDC_OST_Unknown:
			OrderMsg.append("δ֪,");
			break;
			///��δ����
		case CW_FTDC_OST_NotTouched:
			OrderMsg.append("��δ����,");
			break;
			///�Ѵ���
		case CW_FTDC_OST_Touched:
			OrderMsg.append("�Ѵ���,");
			break;
		default:
			OrderMsg.append(&pOrder->OrderStatus);
			OrderMsg.append("_default,");
			break;
		}
		OrderMsg.append(pOrder->StatusMsg);
		OrderMsg.append(" ����δ�յ�����״̬���£���������!");
		m_TradeLog.AddLog(cwTradeLog::enCO, OrderMsg.c_str(), true);
#endif // TRADELOG
		return;
	}

#ifdef NoCancelTooMuchPerTick
	cwAUTOMUTEX mt(m_TradeSpiMutex, true);
	auto it = m_ActiveOrdersMap.find(pOrder->OrderRef);
	if (it != m_ActiveOrdersMap.end())
	{
		if (cwUserCancel_ReqCancel == it->second->UserCancelStatus
			&& abs((int64_t)it->second->UserCancelTime - (int64_t)m_iLatestUpdateTime) < 1000)
		{
#ifdef TRADELOG
			std::string OrderMsg;
			OrderMsg = pOrder->InstrumentID;
			OrderMsg.append(",");
			OrderMsg.append(pOrder->OrderRef);
			OrderMsg.append(",");
			OrderMsg.append(pOrder->OrderSysID);
			OrderMsg.append(",");
			OrderMsg.append(pOrder->UserProductInfo);
			OrderMsg.append(",");
			OrderMsg.append(std::to_string(pOrder->LimitPrice));
			OrderMsg.append(",");
			OrderMsg.append(std::to_string(pOrder->VolumeTotalOriginal));
			OrderMsg.append(",");
			OrderMsg.append(std::to_string(pOrder->VolumeTraded));
			if (pOrder->Direction == CW_FTDC_D_Buy)
			{
				OrderMsg.append(",B,");
			}
			else
			{
				OrderMsg.append(",S,");
			}
			switch (pOrder->CombOffsetFlag[0])
			{
			case CW_FTDC_OF_Open:
				OrderMsg.append("Open,");
				break;
				///ƽ��
			case CW_FTDC_OF_Close:
				OrderMsg.append("Close,");
				break;
				///ƽ��
			case CW_FTDC_OF_CloseToday:
				OrderMsg.append("CloseTd,");
				break;
				///ƽ��
			case CW_FTDC_OF_CloseYesterday:
				OrderMsg.append("CloseYd,");
				break;
			default:
				OrderMsg.append(",");
				break;
			}
			OrderMsg.append(GetInsertOrderTypeString(GetInsertOrderType(pOrder->OrderPriceType,
				pOrder->ContingentCondition, pOrder->TimeCondition, pOrder->VolumeCondition)));
			OrderMsg.append(",");
			OrderMsg.append(pOrder->InsertTime);
			OrderMsg.append(",");
			OrderMsg.append(pOrder->CancelTime);
			OrderMsg.append(",");
			switch (pOrder->OrderStatus)
			{
			case CW_FTDC_OST_AllTraded:
				OrderMsg.append("ȫ���ɽ�,");
				break;
			case CW_FTDC_OST_PartTradedQueueing:
				///���ֳɽ����ڶ�����
				OrderMsg.append("���ֳɽ�(��),");
				break;
			case CW_FTDC_OST_PartTradedNotQueueing:
				///���ֳɽ����ڶ�����
				OrderMsg.append("���ֳɽ�(�Ƕ�),");
				break;
			case CW_FTDC_OST_NoTradeQueueing:
				///δ�ɽ����ڶ�����
				OrderMsg.append("δ�ɽ�(��),");
				break;
			case CW_FTDC_OST_NoTradeNotQueueing:
				///δ�ɽ����ڶ�����
				OrderMsg.append("δ�ɽ�(�Ƕ�),");
				break;
			case CW_FTDC_OST_Canceled:
				///����
				OrderMsg.append("�ѳ���,");
				break;
			case CW_FTDC_OST_AcceptedNoReply:
				///�����ѱ��뽻����δӦ��
				OrderMsg.append("�ѱ�δ��Ӧ,");
				break;
			case CW_FTDC_OST_Unknown:
				OrderMsg.append("δ֪,");
				break;
				///��δ����
			case CW_FTDC_OST_NotTouched:
				OrderMsg.append("��δ����,");
				break;
				///�Ѵ���
			case CW_FTDC_OST_Touched:
				OrderMsg.append("�Ѵ���,");
				break;
			default:
				OrderMsg.append(&pOrder->OrderStatus);
				OrderMsg.append("_default,");
				break;
			}
			OrderMsg.append(pOrder->StatusMsg);
			OrderMsg.append(" �������ύ�����������������ٴ��ύ!");
			m_TradeLog.AddLog(cwTradeLog::enCO, OrderMsg.c_str(), true);
#endif // TRADELOG

			return;
		}
		else
		{
			it->second->UserCancelTime = m_iLatestUpdateTime;
			it->second->UserCancelStatus = cwUserCancel_ReqCancel;
		}
	}
	mt.unlock();
#endif

	CThostFtdcInputOrderActionField CancelOrder;
	memset(&CancelOrder, 0, sizeof(CancelOrder));

	auto InsIt = m_InstrumentMap.find(pOrder->InstrumentID);
	std::string strExchangeID;
	if (InsIt != m_InstrumentMap.end())
	{
		strExchangeID = InsIt->second->ExchangeID;
	}
	else
	{
#ifdef TRADELOG
		std::string OrderMsg;
		OrderMsg = pOrder->InstrumentID;
		OrderMsg.append(",");
		OrderMsg.append(pOrder->OrderRef);
		OrderMsg.append(",");
		OrderMsg.append(pOrder->OrderSysID);
		OrderMsg.append(",");
		OrderMsg.append(pOrder->UserProductInfo);
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pOrder->LimitPrice));
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pOrder->VolumeTotalOriginal));
		OrderMsg.append(",");
		OrderMsg.append(std::to_string(pOrder->VolumeTraded));
		if (pOrder->Direction == CW_FTDC_D_Buy)
		{
			OrderMsg.append(",B,");
		}
		else
		{
			OrderMsg.append(",S,");
		}
		switch (pOrder->CombOffsetFlag[0])
		{
		case CW_FTDC_OF_Open:
			OrderMsg.append("Open,");
			break;
			///ƽ��
		case CW_FTDC_OF_Close:
			OrderMsg.append("Close,");
			break;
			///ƽ��
		case CW_FTDC_OF_CloseToday:
			OrderMsg.append("CloseTd,");
			break;
			///ƽ��
		case CW_FTDC_OF_CloseYesterday:
			OrderMsg.append("CloseYd,");
			break;
		default:
			OrderMsg.append(",");
			break;
		}
		OrderMsg.append(GetInsertOrderTypeString(GetInsertOrderType(pOrder)));
		OrderMsg.append(",");
		OrderMsg.append(pOrder->InsertTime);
		OrderMsg.append(",");
		OrderMsg.append(pOrder->CancelTime);
		OrderMsg.append(",");
		switch (pOrder->OrderStatus)
		{
		case CW_FTDC_OST_AllTraded:
			OrderMsg.append("ȫ���ɽ�,");
			break;
		case CW_FTDC_OST_PartTradedQueueing:
			///���ֳɽ����ڶ�����
			OrderMsg.append("���ֳɽ�(��),");
			break;
		case CW_FTDC_OST_PartTradedNotQueueing:
			///���ֳɽ����ڶ�����
			OrderMsg.append("���ֳɽ�(�Ƕ�),");
			break;
		case CW_FTDC_OST_NoTradeQueueing:
			///δ�ɽ����ڶ�����
			OrderMsg.append("δ�ɽ�(��),");
			break;
		case CW_FTDC_OST_NoTradeNotQueueing:
			///δ�ɽ����ڶ�����
			OrderMsg.append("δ�ɽ�(�Ƕ�),");
			break;
		case CW_FTDC_OST_Canceled:
			///����
			OrderMsg.append("�ѳ���,");
			break;
		case CW_FTDC_OST_AcceptedNoReply:
			///�����ѱ��뽻����δӦ��
			OrderMsg.append("�ѱ�δ��Ӧ,");
			break;
		case CW_FTDC_OST_Unknown:
			OrderMsg.append("δ֪,");
			break;
			///��δ����
		case CW_FTDC_OST_NotTouched:
			OrderMsg.append("��δ����,");
			break;
			///�Ѵ���
		case CW_FTDC_OST_Touched:
			OrderMsg.append("�Ѵ���,");
			break;
		default:
			OrderMsg.append(&pOrder->OrderStatus);
			OrderMsg.append("_default,");
			break;
		}
		OrderMsg.append(pOrder->StatusMsg);
		OrderMsg.append(" �����޷���֪��Լ��Ϣ!");
		m_TradeLog.AddLog(cwTradeLog::enCO, OrderMsg.c_str(), true);
#endif // TRADELOG

		return;
	}

#ifdef _MSC_VER
	strncpy_s(CancelOrder.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(CancelOrder.BrokerID));
	strncpy_s(CancelOrder.UserID, m_ReqUserLoginField.UserID, sizeof(CancelOrder.UserID));
	strncpy_s(CancelOrder.InvestorID, m_strInvestorID.c_str(), sizeof(CancelOrder.InvestorID));
	strcpy_s(CancelOrder.ExchangeID, strExchangeID.c_str());
#else
	strncpy(CancelOrder.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(CancelOrder.BrokerID));
	strncpy(CancelOrder.UserID, m_ReqUserLoginField.UserID, sizeof(CancelOrder.UserID));
	strncpy(CancelOrder.InvestorID, m_strInvestorID.c_str(), sizeof(CancelOrder.InvestorID));
	strcpy(CancelOrder.ExchangeID, strExchangeID.c_str());
#endif // _MSC_VER
	
	CancelOrder.FrontID = m_FrontID;
	CancelOrder.SessionID = m_SessionID;

#ifdef _MSC_VER
	strncpy_s(CancelOrder.OrderRef, pOrder->OrderRef, strlen(pOrder->OrderRef));
	strncpy_s(CancelOrder.OrderSysID, pOrder->OrderSysID, strlen(pOrder->OrderSysID));
#else
	strncpy(CancelOrder.OrderRef, pOrder->OrderRef, strlen(pOrder->OrderRef));
	strncpy(CancelOrder.OrderSysID, pOrder->OrderSysID, strlen(pOrder->OrderSysID));
#endif // _MSC_VER

	CancelOrder.ActionFlag = THOST_FTDC_AF_Delete;

	m_pTraderUserApi->ReqOrderAction(&CancelOrder, ++m_iRequestId);

#ifdef CWCOUTINFO
	m_cwShow.AddLog(" Cancel Order %s %s %d %.3f",
		pOrder->InstrumentID,
		(CW_FTDC_D_Buy == pOrder->Direction ? "B" : "S"),
		pOrder->VolumeTotal,
		pOrder->LimitPrice);

	//if (CW_FTDC_D_Buy == pOrder->Direction)
	//{
	//	std::cout << " Cancel Order " << pOrder->InstrumentID << " B " << pOrder->VolumeTotal << " " << pOrder->LimitPrice << std::endl;
	//}
	//else
	//{
	//	std::cout << " Cancel Order " << pOrder->InstrumentID << " S " << pOrder->VolumeTotal << " " << pOrder->LimitPrice <<  std::endl;
	//}
#endif 

#ifdef TRADELOG
	std::string OrderMsg;
	OrderMsg = pOrder->InstrumentID;
	OrderMsg.append(",");
	OrderMsg.append(pOrder->OrderRef);
	OrderMsg.append(",");
	OrderMsg.append(pOrder->OrderSysID);
	OrderMsg.append(",");
	OrderMsg.append(pOrder->UserProductInfo);
	OrderMsg.append(",");
	OrderMsg.append(std::to_string(pOrder->LimitPrice));
	OrderMsg.append(",");
	OrderMsg.append(std::to_string(pOrder->VolumeTotalOriginal));
	OrderMsg.append(",");
	OrderMsg.append(std::to_string(pOrder->VolumeTraded));
	if (pOrder->Direction == CW_FTDC_D_Buy)
	{
		OrderMsg.append(",B,");
	}
	else
	{
		OrderMsg.append(",S,");
	}
	switch (pOrder->CombOffsetFlag[0])
	{
	case CW_FTDC_OF_Open:
		OrderMsg.append("Open,");
		break;
		///ƽ��
	case CW_FTDC_OF_Close:
		OrderMsg.append("Close,");
		break;
		///ƽ��
	case CW_FTDC_OF_CloseToday:
		OrderMsg.append("CloseTd,");
		break;
		///ƽ��
	case CW_FTDC_OF_CloseYesterday:
		OrderMsg.append("CloseYd,");
		break;
	default:
		OrderMsg.append(",");
		break;
	}
	OrderMsg.append(GetInsertOrderTypeString(GetInsertOrderType(pOrder)));
	OrderMsg.append(",");
	OrderMsg.append(pOrder->InsertTime);
	OrderMsg.append(",");
	OrderMsg.append(pOrder->CancelTime);
	OrderMsg.append(",");
	switch (pOrder->OrderStatus)
	{
	case CW_FTDC_OST_AllTraded:
		OrderMsg.append("ȫ���ɽ�,");
		break;
	case CW_FTDC_OST_PartTradedQueueing:
		///���ֳɽ����ڶ�����
		OrderMsg.append("���ֳɽ�(��),");
		break;
	case CW_FTDC_OST_PartTradedNotQueueing:
		///���ֳɽ����ڶ�����
		OrderMsg.append("���ֳɽ�(�Ƕ�),");
		break;
	case CW_FTDC_OST_NoTradeQueueing:
		///δ�ɽ����ڶ�����
		OrderMsg.append("δ�ɽ�(��),");
		break;
	case CW_FTDC_OST_NoTradeNotQueueing:
		///δ�ɽ����ڶ�����
		OrderMsg.append("δ�ɽ�(�Ƕ�),");
		break;
	case CW_FTDC_OST_Canceled:
		///����
		OrderMsg.append("�ѳ���,");
		break;
	case CW_FTDC_OST_AcceptedNoReply:
		///�����ѱ��뽻����δӦ��
		OrderMsg.append("�ѱ�δ��Ӧ,");
		break;
	case CW_FTDC_OST_Unknown:
		OrderMsg.append("δ֪,");
		break;
		///��δ����
	case CW_FTDC_OST_NotTouched:
		OrderMsg.append("��δ����,");
		break;
		///�Ѵ���
	case CW_FTDC_OST_Touched:
		OrderMsg.append("�Ѵ���,");
		break;
	default:
		OrderMsg.append(&pOrder->OrderStatus);
		OrderMsg.append("_default,");
		break;
	}
	OrderMsg.append(pOrder->StatusMsg);
	m_TradeLog.AddLog(cwTradeLog::enCO, OrderMsg.c_str(), true);
#endif // TRADELOG
}

void cwFtdTradeSpiDemo::OnRtnBulletin(CThostFtdcBulletinField * pBulletin)
{
	if (pBulletin != NULL)
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("\n OnRtnBulletin: %s %s \n %s",
			pBulletin->ExchangeID,
			pBulletin->SendTime,
			pBulletin->Content);

		//std::cout << std::endl;
		//std::cout << " OnRtnBulletin: "
		//	<< pBulletin->ExchangeID << " "
		//	<< pBulletin->SendTime << std::endl;

		//std::cout << pBulletin->Content <<std::endl;
		//std::cout << std::endl;
#endif // CWCOUTINFO

	}


}

void cwFtdTradeSpiDemo::OnRtnTradingNotice(CThostFtdcTradingNoticeInfoField * pTradingNoticeInfo)
{
	if (pTradingNoticeInfo != NULL)
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("\n OnRtnTradingNotice: %s %s \n %s",
			pTradingNoticeInfo->InvestorID,
			pTradingNoticeInfo->SendTime,
			pTradingNoticeInfo->FieldContent);

		//std::cout << std::endl;
		//std::cout << " OnRtnTradingNotice: "
		//	<< pTradingNoticeInfo->InvestorID << " "
		//	<< pTradingNoticeInfo->SendTime << std::endl;

		//std::cout << pTradingNoticeInfo->FieldContent << std::endl;
		//std::cout << std::endl;
#endif // CWCOUTINFO

	}

}


void cwFtdTradeSpiDemo::OnHeartBeatWarning(int nTimeLapse)
{
}



void cwFtdTradeSpiDemo::OnRspUserLogout(CThostFtdcUserLogoutField * pUserLogout, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspUserPasswordUpdate(CThostFtdcUserPasswordUpdateField * pUserPasswordUpdate, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspTradingAccountPasswordUpdate(CThostFtdcTradingAccountPasswordUpdateField * pTradingAccountPasswordUpdate, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspParkedOrderInsert(CThostFtdcParkedOrderField * pParkedOrder, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspParkedOrderAction(CThostFtdcParkedOrderActionField * pParkedOrderAction, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspRemoveParkedOrder(CThostFtdcRemoveParkedOrderField * pRemoveParkedOrder, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspRemoveParkedOrderAction(CThostFtdcRemoveParkedOrderActionField * pRemoveParkedOrderAction, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspExecOrderInsert(CThostFtdcInputExecOrderField * pInputExecOrder, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspExecOrderAction(CThostFtdcInputExecOrderActionField * pInputExecOrderAction, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspForQuoteInsert(CThostFtdcInputForQuoteField * pInputForQuote, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQuoteInsert(CThostFtdcInputQuoteField * pInputQuote, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQuoteAction(CThostFtdcInputQuoteActionField * pInputQuoteAction, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspBatchOrderAction(CThostFtdcInputBatchOrderActionField * pInputBatchOrderAction, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspCombActionInsert(CThostFtdcInputCombActionField * pInputCombAction, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryTradingCode(CThostFtdcTradingCodeField * pTradingCode, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryInstrumentMarginRate(CThostFtdcInstrumentMarginRateField * pInstrumentMarginRate, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryInstrumentCommissionRate(CThostFtdcInstrumentCommissionRateField * pInstrumentCommissionRate, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryExchange(CThostFtdcExchangeField * pExchange, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryProduct(CThostFtdcProductField * pProduct, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryDepthMarketData(CThostFtdcDepthMarketDataField * pDepthMarketData, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQrySettlementInfo(CThostFtdcSettlementInfoField * pSettlementInfo, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryTransferBank(CThostFtdcTransferBankField * pTransferBank, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}



void cwFtdTradeSpiDemo::OnRspQryNotice(CThostFtdcNoticeField * pNotice, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQrySettlementInfoConfirm(CThostFtdcSettlementInfoConfirmField * pSettlementInfoConfirm, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryInvestorPositionCombineDetail(CThostFtdcInvestorPositionCombineDetailField * pInvestorPositionCombineDetail, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryCFMMCTradingAccountKey(CThostFtdcCFMMCTradingAccountKeyField * pCFMMCTradingAccountKey, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryEWarrantOffset(CThostFtdcEWarrantOffsetField * pEWarrantOffset, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryInvestorProductGroupMargin(CThostFtdcInvestorProductGroupMarginField * pInvestorProductGroupMargin, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryExchangeMarginRate(CThostFtdcExchangeMarginRateField * pExchangeMarginRate, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryExchangeMarginRateAdjust(CThostFtdcExchangeMarginRateAdjustField * pExchangeMarginRateAdjust, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryExchangeRate(CThostFtdcExchangeRateField * pExchangeRate, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQrySecAgentACIDMap(CThostFtdcSecAgentACIDMapField * pSecAgentACIDMap, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryProductExchRate(CThostFtdcProductExchRateField * pProductExchRate, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryProductGroup(CThostFtdcProductGroupField * pProductGroup, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryMMInstrumentCommissionRate(CThostFtdcMMInstrumentCommissionRateField * pMMInstrumentCommissionRate, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryMMOptionInstrCommRate(CThostFtdcMMOptionInstrCommRateField * pMMOptionInstrCommRate, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryInstrumentOrderCommRate(CThostFtdcInstrumentOrderCommRateField * pInstrumentOrderCommRate, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryOptionInstrTradeCost(CThostFtdcOptionInstrTradeCostField * pOptionInstrTradeCost, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryOptionInstrCommRate(CThostFtdcOptionInstrCommRateField * pOptionInstrCommRate, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryExecOrder(CThostFtdcExecOrderField * pExecOrder, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryForQuote(CThostFtdcForQuoteField * pForQuote, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryQuote(CThostFtdcQuoteField * pQuote, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryCombInstrumentGuard(CThostFtdcCombInstrumentGuardField * pCombInstrumentGuard, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryCombAction(CThostFtdcCombActionField * pCombAction, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryTransferSerial(CThostFtdcTransferSerialField * pTransferSerial, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryAccountregister(CThostFtdcAccountregisterField * pAccountregister, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRtnInstrumentStatus(CThostFtdcInstrumentStatusField * pInstrumentStatus)
{
	if (pInstrumentStatus == NULL)
	{
		return;
	}
	std::string ExchangeId = pInstrumentStatus->ExchangeID;
	auto it = m_ExchangeStatus.find(ExchangeId);
	if (it == m_ExchangeStatus.end()
		|| it->second != pInstrumentStatus->InstrumentStatus)
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("%s InstrumentStatusChange %s Current Status: %s",
			g_cwGetTradeApiName(m_cwTradeAPIType),
			pInstrumentStatus->ExchangeID,
			g_cwGetInstrumentStatus(GetCtp2CwInstrumentStatusType(pInstrumentStatus->InstrumentStatus)));
		//std::cout << g_cwGetTradeApiName(m_cwTradeAPIType)
		//	<< " InstrumentStatusChange:"
		//	<< pInstrumentStatus->ExchangeID
		//	<< " Current Status: " << g_cwGetInstrumentStatus(GetCtp2CwInstrumentStatusType(pInstrumentStatus->InstrumentStatus)) << std::endl;
#endif
	}
	m_ExchangeStatus[ExchangeId] = pInstrumentStatus->InstrumentStatus;
}

void cwFtdTradeSpiDemo::OnRtnErrorConditionalOrder(CThostFtdcErrorConditionalOrderField * pErrorConditionalOrder)
{
}

void cwFtdTradeSpiDemo::OnRtnExecOrder(CThostFtdcExecOrderField * pExecOrder)
{
}

void cwFtdTradeSpiDemo::OnErrRtnExecOrderInsert(CThostFtdcInputExecOrderField * pInputExecOrder, CThostFtdcRspInfoField * pRspInfo)
{
}

void cwFtdTradeSpiDemo::OnErrRtnExecOrderAction(CThostFtdcExecOrderActionField * pExecOrderAction, CThostFtdcRspInfoField * pRspInfo)
{
}

void cwFtdTradeSpiDemo::OnErrRtnForQuoteInsert(CThostFtdcInputForQuoteField * pInputForQuote, CThostFtdcRspInfoField * pRspInfo)
{
}

void cwFtdTradeSpiDemo::OnRtnQuote(CThostFtdcQuoteField * pQuote)
{
}

void cwFtdTradeSpiDemo::OnErrRtnQuoteInsert(CThostFtdcInputQuoteField * pInputQuote, CThostFtdcRspInfoField * pRspInfo)
{
}

void cwFtdTradeSpiDemo::OnErrRtnQuoteAction(CThostFtdcQuoteActionField * pQuoteAction, CThostFtdcRspInfoField * pRspInfo)
{
}

void cwFtdTradeSpiDemo::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField * pForQuoteRsp)
{
}

void cwFtdTradeSpiDemo::OnRtnCFMMCTradingAccountToken(CThostFtdcCFMMCTradingAccountTokenField * pCFMMCTradingAccountToken)
{
}

void cwFtdTradeSpiDemo::OnErrRtnBatchOrderAction(CThostFtdcBatchOrderActionField * pBatchOrderAction, CThostFtdcRspInfoField * pRspInfo)
{
}

void cwFtdTradeSpiDemo::OnRtnCombAction(CThostFtdcCombActionField * pCombAction)
{
}

void cwFtdTradeSpiDemo::OnErrRtnCombActionInsert(CThostFtdcInputCombActionField * pInputCombAction, CThostFtdcRspInfoField * pRspInfo)
{
}

void cwFtdTradeSpiDemo::OnRspQryContractBank(CThostFtdcContractBankField * pContractBank, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryParkedOrder(CThostFtdcParkedOrderField * pParkedOrder, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryParkedOrderAction(CThostFtdcParkedOrderActionField * pParkedOrderAction, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryTradingNotice(CThostFtdcTradingNoticeField * pTradingNotice, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryBrokerTradingParams(CThostFtdcBrokerTradingParamsField * pBrokerTradingParams, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQryBrokerTradingAlgos(CThostFtdcBrokerTradingAlgosField * pBrokerTradingAlgos, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQueryCFMMCTradingAccountToken(CThostFtdcQueryCFMMCTradingAccountTokenField * pQueryCFMMCTradingAccountToken, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRtnFromBankToFutureByBank(CThostFtdcRspTransferField * pRspTransfer)
{
}

void cwFtdTradeSpiDemo::OnRtnFromFutureToBankByBank(CThostFtdcRspTransferField * pRspTransfer)
{
}

void cwFtdTradeSpiDemo::OnRtnRepealFromBankToFutureByBank(CThostFtdcRspRepealField * pRspRepeal)
{
}

void cwFtdTradeSpiDemo::OnRtnRepealFromFutureToBankByBank(CThostFtdcRspRepealField * pRspRepeal)
{
}

void cwFtdTradeSpiDemo::OnRtnFromBankToFutureByFuture(CThostFtdcRspTransferField * pRspTransfer)
{
}

void cwFtdTradeSpiDemo::OnRtnFromFutureToBankByFuture(CThostFtdcRspTransferField * pRspTransfer)
{
}

void cwFtdTradeSpiDemo::OnRtnRepealFromBankToFutureByFutureManual(CThostFtdcRspRepealField * pRspRepeal)
{
}

void cwFtdTradeSpiDemo::OnRtnRepealFromFutureToBankByFutureManual(CThostFtdcRspRepealField * pRspRepeal)
{
}

void cwFtdTradeSpiDemo::OnRtnQueryBankBalanceByFuture(CThostFtdcNotifyQueryAccountField * pNotifyQueryAccount)
{
}

void cwFtdTradeSpiDemo::OnErrRtnBankToFutureByFuture(CThostFtdcReqTransferField * pReqTransfer, CThostFtdcRspInfoField * pRspInfo)
{
}

void cwFtdTradeSpiDemo::OnErrRtnFutureToBankByFuture(CThostFtdcReqTransferField * pReqTransfer, CThostFtdcRspInfoField * pRspInfo)
{
}

void cwFtdTradeSpiDemo::OnErrRtnRepealBankToFutureByFutureManual(CThostFtdcReqRepealField * pReqRepeal, CThostFtdcRspInfoField * pRspInfo)
{
}

void cwFtdTradeSpiDemo::OnErrRtnRepealFutureToBankByFutureManual(CThostFtdcReqRepealField * pReqRepeal, CThostFtdcRspInfoField * pRspInfo)
{
}

void cwFtdTradeSpiDemo::OnErrRtnQueryBankBalanceByFuture(CThostFtdcReqQueryAccountField * pReqQueryAccount, CThostFtdcRspInfoField * pRspInfo)
{
}

void cwFtdTradeSpiDemo::OnRtnRepealFromBankToFutureByFuture(CThostFtdcRspRepealField * pRspRepeal)
{
}

void cwFtdTradeSpiDemo::OnRtnRepealFromFutureToBankByFuture(CThostFtdcRspRepealField * pRspRepeal)
{
}

void cwFtdTradeSpiDemo::OnRspFromBankToFutureByFuture(CThostFtdcReqTransferField * pReqTransfer, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspFromFutureToBankByFuture(CThostFtdcReqTransferField * pReqTransfer, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRspQueryBankAccountMoneyByFuture(CThostFtdcReqQueryAccountField * pReqQueryAccount, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{
}

void cwFtdTradeSpiDemo::OnRtnOpenAccountByBank(CThostFtdcOpenAccountField * pOpenAccount)
{
}

void cwFtdTradeSpiDemo::OnRtnCancelAccountByBank(CThostFtdcCancelAccountField * pCancelAccount)
{
}

void cwFtdTradeSpiDemo::OnRtnChangeAccountByBank(CThostFtdcChangeAccountField * pChangeAccount)
{
}

#ifdef 	CW_API_6_5_1
void cwFtdTradeSpiDemo::OnRspQryClassifiedInstrument(CThostFtdcInstrumentField * pInstrument, CThostFtdcRspInfoField * pRspInfo, int nRequestID, bool bIsLast)
{

}
#endif

void cwFtdTradeSpiDemo::RegisterBasicStrategy(cwBasicStrategy * pBasicStrategy, void * pSpi)
{
	m_pBasicStrategy = pBasicStrategy;
	if (m_pBasicStrategy != NULL)
	{
		if (pSpi == NULL)
		{
			m_pBasicStrategy->_SetTradeSpi(m_cwTradeAPIType, dynamic_cast<cwBasicTradeSpi*>(this));
		}
		else
		{
			m_pBasicStrategy->_SetTradeSpi(m_cwTradeAPIType, pSpi);
		}
	}
}

void cwFtdTradeSpiDemo::Connect(const char * pszFrontAddress, const char * pszDllPath)
{
#ifdef _MSC_VER
	strncpy_s(m_szTradeFrount, pszFrontAddress, strlen(pszFrontAddress));
#else
	strncpy(m_szTradeFrount, pszFrontAddress, strlen(pszFrontAddress));
#endif // _MSC_VER

	if (m_pTraderUserApi)
	{
		DisConnect();
	}
	char exeFullPath[MAX_PATH];
	memset(exeFullPath, 0, sizeof(exeFullPath));
	struct stat buf;

#ifdef _MSC_VER
	TCHAR TexeFullPath[MAX_PATH];
	memset(TexeFullPath, 0, sizeof(TexeFullPath));
	::GetModuleFileName(NULL, TexeFullPath, MAX_PATH);

	int iLength;
	//��ȡ�ֽڳ���   
	iLength = WideCharToMultiByte(CP_ACP, 0, TexeFullPath, -1, NULL, 0, NULL, NULL);
	//��tcharֵ����_char    
	WideCharToMultiByte(CP_ACP, 0, TexeFullPath, -1, exeFullPath, iLength, NULL, NULL);

	std::string strFullPath = exeFullPath;

#ifdef CW_USING_DYNAMIC_LOADING_DLL
	if (pszDllPath == NULL
		|| strlen(pszDllPath) == 0)
	{
		strFullPath = strFullPath.substr(0, strFullPath.rfind('\\'));
		strFullPath.append("\\thosttraderapi_se.dll");
	}
	else
	{
		strFullPath = pszDllPath;
	}

	memset(TexeFullPath, 0, sizeof(TexeFullPath));

	iLength = MultiByteToWideChar(CP_ACP, 0, strFullPath.c_str(), strFullPath.length(), NULL, 0);
	MultiByteToWideChar(CP_ACP, 0, strFullPath.c_str(), strFullPath.length(), TexeFullPath, iLength);

	typedef CThostFtdcTraderApi * (*CreatApi)(const char *);
	CreatApi pCreatApi = NULL;

	typedef const char * (*GetVersion)();
	GetVersion pGetVersion = NULL;

	m_cwhDllInstance = LoadLibrary(TexeFullPath);
	if (m_cwhDllInstance == NULL)
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog(" Load CTP API DLL Library Failed! Please Check !");
#endif
		cwSleep(3000);
		exit(-1);
	}

	pCreatApi = (CreatApi)GetProcAddress(m_cwhDllInstance, "?CreateFtdcTraderApi@CThostFtdcTraderApi@@SAPAV1@PBD@Z");
	if (pCreatApi == NULL)
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog(" Load CreateFtdcTraderApi Failed! Please Check !");
#endif
		cwSleep(3000);
		exit(-1);
	}

	pGetVersion = (GetVersion)GetProcAddress(m_cwhDllInstance, "?GetApiVersion@CThostFtdcTraderApi@@SAPBDXZ");

	strFullPath = exeFullPath;
#endif

	strFullPath = strFullPath.substr(0, strFullPath.rfind('\\'));
	strFullPath.append("\\Instrument.xml");
	this->SetInstrumentDataFileName(strFullPath.c_str());
	//this->GetInstrumentDataFromFile();

	strFullPath = strFullPath.substr(0, strFullPath.rfind('\\'));
	strFullPath.append("\\Trade\\");

	if (stat(strFullPath.c_str(), &buf) != 0)
	{
		_mkdir(strFullPath.c_str());
	}

	strFullPath.append(std::to_string(m_iTradeAPIIndex));
	strFullPath.append("\\");
#else
	size_t cnt = readlink("/proc/self/exe", exeFullPath, MAX_PATH);
	if (cnt < 0 || cnt >= MAX_PATH)
	{
		printf("***Error***\n");
		exit(-1);
	}

	std::string strFullPath = exeFullPath;

#ifdef CW_USING_DYNAMIC_LOADING_DLL

	if (pszDllPath == NULL)
	{
		strFullPath = strFullPath.substr(0, strFullPath.rfind('\\'));
		strFullPath.append("\\thosttraderapi_se.so");
	}
	else
	{
		strFullPath = pszDllPath;
	}

	m_cwhDllInstance = dlopen(strFullPath.c_str(), RTLD_LAZY);
	if (m_cwhDllInstance == NULL)
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog(" Load CTP API DLL Library Failed! Please Check !");
#endif
		cwSleep(50000);
		exit(-1);
	}

	typedef CThostFtdcTraderApi * (__cdecl * CreatApi)(const char *);
	CreatApi pCreatApi = NULL;

	typedef const char * (*GetVersion)();
	GetVersion pGetVersion = NULL;

	pCreatApi = (CreatApi)dlsym(m_cwhDllInstance, "_ZN19CThostFtdcTraderApi19CreateFtdcTraderApiEPKc");
	if (pCreatApi == NULL)
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog(" Load CreateFtdcTraderApi Failed! Please Check !");
#endif
		cwSleep(3000);
		exit(-1);
	}

	pGetVersion = (GetVersion)GetProcAddress(m_cwhDllInstance, "_ZN19CThostFtdcTraderApi13GetApiVersionEv");

	strFullPath = exeFullPath;
#endif

	strFullPath = strFullPath.substr(0, strFullPath.rfind('/'));
	strFullPath.append("/Instrument.xml");
	this->SetInstrumentDataFileName(strFullPath.c_str());
	//this->GetInstrumentDataFromFile();

	strFullPath = strFullPath.substr(0, strFullPath.rfind('/'));
	strFullPath.append("/Trade/");

	if (stat(strFullPath.c_str(), &buf) != 0)
	{
		mkdir(strFullPath.c_str(), 0755);
	}

	strFullPath.append(std::to_string(m_iTradeAPIIndex));
	strFullPath.append("/");
#endif

	if (stat(strFullPath.c_str(), &buf) != 0)
	{
#ifdef _MSC_VER
		_mkdir(strFullPath.c_str());
#else
		mkdir(strFullPath.c_str(), 0755);
#endif // _MSC_VER
	}

#ifdef CW_USING_DYNAMIC_LOADING_DLL
	m_pTraderUserApi = pCreatApi(strFullPath.c_str());
#else
	m_pTraderUserApi = CThostFtdcTraderApi::CreateFtdcTraderApi(strFullPath.c_str());
#endif

	if (m_pTraderUserApi == NULL
		|| strlen(m_szTradeFrount) <= 0)
	{
		return;
	}

#ifdef CWCOUTINFO
#ifdef CW_USING_DYNAMIC_LOADING_DLL
	if (pGetVersion != NULL)
	{
		m_cwShow.AddLog("%s Version: %s",
			g_cwGetTradeApiName(m_cwTradeAPIType),
			pGetVersion());
	}
#else
	m_cwShow.AddLog("%s Version: %s",
		g_cwGetTradeApiName(m_cwTradeAPIType),
		m_pTraderUserApi->GetApiVersion());
	//std::cout << g_cwGetTradeApiName(m_cwTradeAPIType) << " Version:" << m_pTraderUserApi->GetApiVersion() << std::endl;
#endif
#endif
	m_pTraderUserApi->RegisterSpi((CThostFtdcTraderSpi *)this);

	m_pTraderUserApi->SubscribePublicTopic(m_iResumeType);
	m_pTraderUserApi->SubscribePrivateTopic(m_iResumeType);

#ifdef CWCOUTINFO
	m_cwShow.AddLog("%s FrontAddr: %s",
		g_cwGetTradeApiName(m_cwTradeAPIType),
		pszFrontAddress);
#endif
	
	m_pTraderUserApi->RegisterFront(m_szTradeFrount);
	m_pTraderUserApi->Init();
}

void cwFtdTradeSpiDemo::DisConnect()
{
	if (m_pTraderUserApi)
	{
#ifdef TRADELOG
		m_TradeLog.AddLog(cwTradeLog::enMsg, ",,,,,,,,,,,,,DisConnect!!", true);
#endif // TRADELOG

		m_bReqQryThreadRun = false;

		m_pTraderUserApi->RegisterSpi(NULL);
		m_pTraderUserApi->Release();

		m_pTraderUserApi = NULL;
	}
	m_CurrentStatus = TradeServerStatus::Status_UnConnected;
	cwBasicTradeSpi::Reset();
}


void cwFtdTradeSpiDemo::WaitForFinish()
{
	m_pTraderUserApi->Join();
}


void cwFtdTradeSpiDemo::SetUserLoginField(const char * szBrokerID, const char * szUserID, const char * szPassword, const char * szUserProductInfo)
{
	if (szUserProductInfo != NULL)
	{
#ifdef _MSC_VER
		strcpy_s(m_ReqUserLoginField.UserProductInfo, szUserProductInfo);
#else
		strcpy(m_ReqUserLoginField.UserProductInfo, szUserProductInfo);
#endif // _MSC_VER
	}
	else
	{
#ifdef _MSC_VER
		strcpy_s(m_ReqUserLoginField.UserProductInfo, INTERFACENAME);
#else
		strcpy(m_ReqUserLoginField.UserProductInfo, INTERFACENAME);
#endif // _MSC_VER
	}

	if (szBrokerID != NULL)
	{
#ifdef _MSC_VER
		strcpy_s(m_ReqUserLoginField.BrokerID, szBrokerID);
#else
		strcpy(m_ReqUserLoginField.BrokerID, szBrokerID);
#endif // _MSC_VER
	}
	if (szUserID != NULL)
	{
#ifdef _MSC_VER
		strcpy_s(m_ReqUserLoginField.UserID, szUserID);
#else
		strcpy(m_ReqUserLoginField.UserID, szUserID);
#endif // _MSC_VER
	}
	if (szPassword != NULL)
	{
#ifdef _MSC_VER
		strcpy_s(m_ReqUserLoginField.Password, szPassword);
#else
		strcpy(m_ReqUserLoginField.Password, szPassword);
#endif // _MSC_VER
	}
}

void cwFtdTradeSpiDemo::SetUserLoginField(CThostFtdcReqUserLoginField& reqUserLoginField)
{
#ifdef _MSC_VER
	memcpy_s(&m_ReqUserLoginField, sizeof(m_ReqUserLoginField), &reqUserLoginField, sizeof(reqUserLoginField));
#else
	memcpy(&m_ReqUserLoginField, &reqUserLoginField, sizeof(reqUserLoginField));
#endif // _MSC_VER
}

void cwFtdTradeSpiDemo::SetAuthenticateInfo(const char * szAppID, const char * szAuthCode)
{
	if (szAppID != NULL)
	{
#ifdef _MSC_VER
		strcpy_s(m_AppID, szAppID);
#else
		strcpy(m_AppID, szAppID);
#endif // _MSC_VER
	}

	if (szAuthCode != NULL)
	{
#ifdef _MSC_VER
		strcpy_s(m_AuthCode, szAuthCode);
#else
		strcpy(m_AuthCode, szAuthCode);
#endif // _MSC_VER
	}
}

void cwFtdTradeSpiDemo::PriceUpdate(cwMarketDataPtr pPriceData)
{
#ifdef NoCancelTooMuchPerTick
	cwMarketTime time(pPriceData->UpdateTime, pPriceData->UpdateMillisec);
	m_iLatestUpdateTime = (uint32_t)time.GetTotalMilliSecond();
#endif // NoCancelTooMuchPerTick

#ifdef UPDATE_ORDERRANKED
	cwAUTOMUTEX mt(m_TradeSpiMutex, true);
	if (m_ActiveOrdersMap.size() == 0)
	{
		return;
	}

	mt.unlock();
	m_TickTradeManger.UpdatePrice(pPriceData);
	std::map<uint32_t, uint32_t>& CurrentTradeMap = m_TickTradeManger.GetCurrentTradeMap();
	
	std::string strInstrumentID = pPriceData->InstrumentID;
	std::map<std::string, cwOrderPtr>::iterator it;
	uint32_t OrderPrice = 0; 
	mt.lock();

	bool bNo5LevelData = (pPriceData->BidVolume2 <= 0 && pPriceData->AskVolume2 <= 0);


	for (it = m_ActiveOrdersMap.begin(); it != m_ActiveOrdersMap.end(); it++)
	{
		if (strInstrumentID == it->second->InstrumentID)
		{
			OrderPrice = (uint32_t)((it->second->LimitPrice + 0.000001) * 1000);
			if (CurrentTradeMap.find(OrderPrice) != CurrentTradeMap.end())
			{
				it->second->iRanked -= CurrentTradeMap[OrderPrice];
				m_bHasActiveOrdersChanged = true;
			}

			if (it->second->Direction == CW_FTDC_D_Buy)
			{	
				if (bNo5LevelData)
				{
					if (it->second->iRanked > 0 && fabs(it->second->LimitPrice - pPriceData->BidPrice1) < 0.000001)
					{
						if (it->second->iRanked > (int)pPriceData->BidVolume1)
						{
							it->second->iRanked = pPriceData->BidVolume1;
							m_bHasActiveOrdersChanged = true;
						}
					}
				}
				else
				{
					for (int i = 0; i < MARKET_PRICE_DEPTH; i++)
					{
						if (it->second->iRanked > 0 && fabs(it->second->LimitPrice - pPriceData->BuyLevel[i].Price) < 0.000001)
						{
							if (it->second->iRanked > (int)pPriceData->BuyLevel[i].Volume)
							{
								it->second->iRanked = pPriceData->BuyLevel[i].Volume;
								m_bHasActiveOrdersChanged = true;
							}
							break;
						}
					}
				}
			}
			else
			{
				if (bNo5LevelData)
				{
					if (it->second->iRanked > 0 && fabs(it->second->LimitPrice - pPriceData->AskPrice1) < 0.000001)
					{
						if (it->second->iRanked > (int)pPriceData->AskVolume1)
						{
							it->second->iRanked = pPriceData->AskVolume1;
							m_bHasActiveOrdersChanged = true;
						}
					}
				}
				else
				{
					for (int i = 0; i < MARKET_PRICE_DEPTH; i++)
					{
						if (it->second->iRanked > 0 && fabs(it->second->LimitPrice - pPriceData->SellLevel[i].Price) < 0.000001)
						{
							if (it->second->iRanked > (int)pPriceData->SellLevel[i].Volume)
							{
								it->second->iRanked = pPriceData->SellLevel[i].Volume;
								m_bHasActiveOrdersChanged = true;
							}
							break;
						}
					}
				}
			}
		}
	}
#endif
}

bool cwFtdTradeSpiDemo::MyReqFunction(cwReqType nType, void * pData)
{
	int iReqCnt = 0;
	int iRet = 0;

Req:
	if (pData == NULL || m_pTraderUserApi == NULL)
	{
		return false;
	}
	iReqCnt++;
	switch (nType)
	{
	case cwFtdTradeSpiDemo::cwReqAuthenticate:
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi: ReqAuthenticate");
		//std::cout << "TradeSpi: ReqAuthenticate" << std::endl;
#endif

		iRet = m_pTraderUserApi->ReqAuthenticate((CThostFtdcReqAuthenticateField *)pData, ++m_iRequestId);
		break;
	}
	case cwFtdTradeSpiDemo::cwReqUserLogin:
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi: ReqUserLogin");
		//std::cout << "TradeSpi: ReqUserLogin" << std::endl;
#endif

		iRet = m_pTraderUserApi->ReqUserLogin((CThostFtdcReqUserLoginField *)pData, ++m_iRequestId);
		break;
	}
	case cwFtdTradeSpiDemo::cwReqQryInvestor:
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi:ReqQryInvestor");
		//std::cout << "TradeSpi:ReqQryInvestor" << std::endl;
#endif
		iRet = m_pTraderUserApi->ReqQryInvestor((CThostFtdcQryInvestorField *)pData, ++m_iRequestId);
		break;
	}
	case cwFtdTradeSpiDemo::cwReqSettlementInfoConfirm:
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi:ReqSettlementInfoConfirm");
		//std::cout << "TradeSpi:ReqSettlementInfoConfirm" << std::endl;
#endif
		iRet = m_pTraderUserApi->ReqSettlementInfoConfirm((CThostFtdcSettlementInfoConfirmField *)pData, ++m_iRequestId);
		break;
	}
	case cwFtdTradeSpiDemo::cwReqQryInstrument:
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi:ReqQryInstrument");
		//std::cout << "TradeSpi:ReqQryInstrument" << std::endl;
#endif
		iRet = m_pTraderUserApi->ReqQryInstrument((CThostFtdcQryInstrumentField *)pData, ++m_iRequestId);
		break;
	}
#ifdef 	CW_API_6_5_1
	case cwFtdTradeSpiDemo::cwReqQryClassifiedInstrument:
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi:cwReqQryClassifiedInstrument");
		//std::cout << "TradeSpi:ReqQryInstrument" << std::endl;
#endif
		iRet = m_pTraderUserApi->ReqQryClassifiedInstrument((CThostFtdcQryClassifiedInstrumentField *)pData, ++m_iRequestId);
		break;
	}
#endif
	case cwFtdTradeSpiDemo::cwReqQryTradingAccount:
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi:ReqQryTradingAccount");
		//std::cout << "TradeSpi:ReqQryTradingAccount" << std::endl;
#endif
		iRet = m_pTraderUserApi->ReqQryTradingAccount((CThostFtdcQryTradingAccountField *)pData, ++m_iRequestId);
		break;
	}
	case cwFtdTradeSpiDemo::cwRspQryInvestorPosition:
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi:RspQryInvestorPosition");
		//std::cout << "TradeSpi:RspQryInvestorPosition" << std::endl;
#endif
		m_bIsQryingPosition = true;

		iRet = m_pTraderUserApi->ReqQryInvestorPosition((CThostFtdcQryInvestorPositionField *)pData, ++m_iRequestId);
		break;
	}
	case cwFtdTradeSpiDemo::cwRspQryInvestorPositionDetail:
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi:RspQryInvestorPositionDetail");
		//std::cout << "TradeSpi:RspQryInvestorPositionDetail" << std::endl;
#endif

		iRet = m_pTraderUserApi->ReqQryInvestorPositionDetail((CThostFtdcQryInvestorPositionDetailField *)pData, ++m_iRequestId);
		break;
	}
	case cwFtdTradeSpiDemo::cwReqQryOrder:
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi:ReqQryOrder");
		//std::cout << "TradeSpi:ReqQryOrder" << std::endl;
#endif
		iRet = m_pTraderUserApi->ReqQryOrder((CThostFtdcQryOrderField*)pData, ++m_iRequestId);
		break;
	}
	case cwFtdTradeSpiDemo::cwReqQryTrade:
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi:ReqQryTrade");
		//std::cout << "TradeSpi:ReqQryTrade" << std::endl;
#endif
		iRet = m_pTraderUserApi->ReqQryTrade((CThostFtdcQryTradeField *)pData, ++m_iRequestId);
		break;
	}
	case cwFtdTradeSpiDemo::cwReqSettlementInfo:
	{
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi:ReqQrySettlementInfo %s", ((CThostFtdcQrySettlementInfoField *)pData)->TradingDay);
		//std::cout << "TradeSpi:ReqQrySettlementInfo" << std::endl;
#endif
		iRet = m_pTraderUserApi->ReqQrySettlementInfo((CThostFtdcQrySettlementInfoField *)pData, ++m_iRequestId);
		break;
	}
	default:
		break;
	}
	
	switch (iRet)
	{
	case 0:
		m_bIsQryingPosition = false;
		return true;
		//success ���ͳɹ�
		break;
	case -1:
		//Send failed because of network ����ԭ����ʧ��
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi:Send failed because of network ");
		//std::cout << "TradeSpi:Send failed because of network " << std::endl;
#endif
		break;
	case -2:
		//too much request to handle δ���������������������
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi:too much request to handle ");
		//std::cout << "TradeSpi:too much request to handle " << std::endl;
#endif
		if (iReqCnt < 5)
		{
			cwSleep(1000);
			goto Req;
		}
		break;
	case -3:
		//Send request too much per second ÿ�뷢��������
#ifdef CWCOUTINFO
		m_cwShow.AddLog("TradeSpi:Send request too much per second ");
		//std::cout << "TradeSpi:Send request too much per second " << std::endl;
#endif
		if (iReqCnt < 5)
		{
			cwSleep(1000);
			goto Req;
		}
		break;
	default:
		break;
	}

	m_bIsQryingPosition = false;
	return false;
}

bool cwFtdTradeSpiDemo::AddMyReqToList(cwReqType nType)
{
	cwAUTOMUTEX mt(m_PrioritizedReqListMutex, true);

	m_PrioritizedReqList.push_back(nType);

	return true;
}

#ifdef _MSC_VER
#pragma region CTPDefine2CWDefine
#endif // _MSC_VER

cwFtdcProductClassType cwFtdTradeSpiDemo::GetCtp2CwProductClassType(TThostFtdcProductClassType productclassType)
{
	switch (productclassType)
	{
		///�ڻ�
	case THOST_FTDC_PC_Futures:
		return CW_FTDC_PC_Futures;
		///�ڻ���Ȩ
	case THOST_FTDC_PC_Options:
		return CW_FTDC_PC_Options;
		///���
	case THOST_FTDC_PC_Combination:
		return CW_FTDC_PC_Combination;
		///����
	case THOST_FTDC_PC_Spot:
		return CW_FTDC_PC_Spot;
		///��ת��
	case THOST_FTDC_PC_EFP:
		return CW_FTDC_PC_EFP;
		///�ֻ���Ȩ
	case THOST_FTDC_PC_SpotOption:
		return CW_FTDC_PC_SpotOption;
#ifdef 	CW_API_6_5_1
		///TAS��Լ
	case THOST_FTDC_PC_TAS:
		return CW_FTDC_PC_TAS;
		///����ָ��
	case THOST_FTDC_PC_MI:
		return CW_FTDC_PC_MI;
#endif
	default:
		return CW_FTDC_PC_UnKnow;
		break;
	}
}
cwFtdcDirectionType cwFtdTradeSpiDemo::GetCtp2CwDirectionType(TThostFtdcDirectionType direction, bool bPosition)
{
	if (bPosition)
	{
		return direction == THOST_FTDC_PD_Long ? CW_FTDC_D_Buy : CW_FTDC_D_Sell;
	}
	return direction == THOST_FTDC_D_Buy ? CW_FTDC_D_Buy : CW_FTDC_D_Sell;
}

TThostFtdcDirectionType cwFtdTradeSpiDemo::GetCw2CtpDirectionType(cwFtdcDirectionType direction)
{
	return direction == CW_FTDC_D_Buy ? THOST_FTDC_D_Buy : THOST_FTDC_D_Sell;
}

cwFtdcHedgeFlagType cwFtdTradeSpiDemo::GetCtp2CwHedgeFlagType(TThostFtdcHedgeFlagType hedge)
{
	switch (hedge)
	{
		///Ͷ��
	case THOST_FTDC_HF_Speculation:
		return CW_FTDC_HF_Speculation;
		///����
	case THOST_FTDC_HF_Arbitrage:
		return CW_FTDC_HF_Arbitrage;
		///�ױ�
	case THOST_FTDC_HF_Hedge:
		return CW_FTDC_HF_Hedge;
		///������
	case THOST_FTDC_HF_MarketMaker:
		return CW_FTDC_HF_MarketMaker;
	default:
		break;
	}
	return cwFtdcHedgeFlagType();
}

cwFtdcOffsetFlagType cwFtdTradeSpiDemo::GetCtp2CwOffsetFlagType(TThostFtdcOffsetFlagType Offset)
{
	switch (Offset)
	{
	case THOST_FTDC_OF_Open:
		///����
		return CW_FTDC_OF_Open;
	case THOST_FTDC_OF_Close:
		///ƽ��
		return CW_FTDC_OF_Close;
	case THOST_FTDC_OF_ForceClose:
		///ǿƽ
		return CW_FTDC_OF_ForceClose;
	case THOST_FTDC_OF_CloseToday:
		///ƽ��
		return CW_FTDC_OF_CloseToday;
	case THOST_FTDC_OF_CloseYesterday:
		///ƽ��
		return CW_FTDC_OF_CloseYesterday;
	default:
		break;
	}
	return cwFtdcOffsetFlagType();
}

cwFtdcOrderPriceType cwFtdTradeSpiDemo::GetCtp2CwOrderPriceType(TThostFtdcOrderPriceTypeType orderpricetype)
{
	switch (orderpricetype)
	{
	case THOST_FTDC_OPT_AnyPrice:
		///�����
		return CW_FTDC_OPT_AnyPrice;
	case THOST_FTDC_OPT_LimitPrice:
		///�޼�
		return CW_FTDC_OPT_LimitPrice;
	case THOST_FTDC_OPT_BestPrice:
		///���ż�
		return CW_FTDC_OPT_BestPrice;
	case THOST_FTDC_OPT_LastPrice:
		///���¼�
		return CW_FTDC_OPT_LastPrice;
	case THOST_FTDC_OPT_LastPricePlusOneTicks:
		///���¼۸����ϸ�1��ticks
		return CW_FTDC_OPT_LastPricePlusOneTicks;
	case THOST_FTDC_OPT_LastPricePlusTwoTicks:
		///���¼۸����ϸ�2��ticks
		return CW_FTDC_OPT_LastPricePlusTwoTicks;
	case THOST_FTDC_OPT_LastPricePlusThreeTicks:
		///���¼۸����ϸ�3��ticks
		return CW_FTDC_OPT_LastPricePlusThreeTicks;
	case THOST_FTDC_OPT_AskPrice1:
		///��һ��
		return CW_FTDC_OPT_AskPrice1;
	case THOST_FTDC_OPT_AskPrice1PlusOneTicks:
		///��һ�۸����ϸ�1��ticks
		return CW_FTDC_OPT_AskPrice1PlusOneTicks;
	case THOST_FTDC_OPT_AskPrice1PlusTwoTicks:
		///��һ�۸����ϸ�2��ticks
		return CW_FTDC_OPT_AskPrice1PlusTwoTicks;
	case THOST_FTDC_OPT_AskPrice1PlusThreeTicks:
		///��һ�۸����ϸ�3��ticks
		return CW_FTDC_OPT_AskPrice1PlusThreeTicks;
	case THOST_FTDC_OPT_BidPrice1:
		///��һ��
		return CW_FTDC_OPT_BidPrice1;
	case THOST_FTDC_OPT_BidPrice1PlusOneTicks:
		///��һ�۸����ϸ�1��ticks
		return CW_FTDC_OPT_BidPrice1PlusOneTicks;
	case THOST_FTDC_OPT_BidPrice1PlusTwoTicks:
		///��һ�۸����ϸ�2��ticks
		return CW_FTDC_OPT_BidPrice1PlusTwoTicks;
	case THOST_FTDC_OPT_BidPrice1PlusThreeTicks:
		///��һ�۸����ϸ�3��ticks
		return CW_FTDC_OPT_BidPrice1PlusThreeTicks;
	case THOST_FTDC_OPT_FiveLevelPrice:
		///�嵵��
		return CW_FTDC_OPT_FiveLevelPrice;
	default:
		break;
	}
	
	return cwFtdcOrderPriceType();
}

cwFtdcTimeConditionType cwFtdTradeSpiDemo::GetCtp2CwTimeConditionType(TThostFtdcTimeConditionType timeconditiontype)
{
	switch (timeconditiontype)
	{
	case THOST_FTDC_TC_IOC:
		///������ɣ�������
		return CW_FTDC_TC_IOC;
		break;
	case THOST_FTDC_TC_GFS:
		///������Ч
		return CW_FTDC_TC_GFS;
		break;
	case THOST_FTDC_TC_GFD:
		///������Ч
		return CW_FTDC_TC_GFD;
		break;
	case THOST_FTDC_TC_GTD:
		///ָ������ǰ��Ч
		return CW_FTDC_TC_GTD;
		break;
	case THOST_FTDC_TC_GTC:
		///����ǰ��Ч
		return CW_FTDC_TC_GTC;
		break;
	case THOST_FTDC_TC_GFA:
		///���Ͼ�����Ч
		return CW_FTDC_TC_GFA;
		break;
	default:
		break;
	}
	return cwFtdcTimeConditionType();
}

cwFtdcVolumeConditionType cwFtdTradeSpiDemo::GetCtp2CwVolumeConditionType(TThostFtdcVolumeConditionType volumeconditiontype)
{
	switch (volumeconditiontype)
	{
	case THOST_FTDC_VC_AV:
		///�κ�����
		return CW_FTDC_VC_AV;
	case THOST_FTDC_VC_MV:
		///��С����
		return CW_FTDC_VC_MV;
	case THOST_FTDC_VC_CV:
		///ȫ������
		return CW_FTDC_VC_CV;
	default:
		break;
	}
	return cwFtdcVolumeConditionType();
}

cwFtdcForceCloseReasonType cwFtdTradeSpiDemo::GetCtd2CwForceCloseReasonType(TThostFtdcForceCloseReasonType forceclosereasontype)
{
	switch (forceclosereasontype)
	{
	case THOST_FTDC_FCC_NotForceClose:
		///��ǿƽ
		return CW_FTDC_FCC_NotForceClose;
	case THOST_FTDC_FCC_LackDeposit:
		///�ʽ���
		return CW_FTDC_FCC_LackDeposit;
	case THOST_FTDC_FCC_ClientOverPositionLimit:
		///�ͻ�����
		return CW_FTDC_FCC_ClientOverPositionLimit;
	case THOST_FTDC_FCC_MemberOverPositionLimit:
		///��Ա����
		return CW_FTDC_FCC_MemberOverPositionLimit;
	case THOST_FTDC_FCC_NotMultiple:
		///�ֲַ�������
		return CW_FTDC_FCC_NotMultiple;
	case THOST_FTDC_FCC_Violation:
		///Υ��
		return CW_FTDC_FCC_Violation;
	case THOST_FTDC_FCC_Other:
		///����
		return CW_FTDC_FCC_Other;
	case THOST_FTDC_FCC_PersonDeliv:
		///��Ȼ���ٽ�����
		return CW_FTDC_FCC_PersonDeliv;
	default:
		break;
	}
	return cwFtdcForceCloseReasonType();
}

cwFtdcOrderSourceType cwFtdTradeSpiDemo::GetCtp2CwOrderSourceType(TThostFtdcOrderSourceType ordersource)
{
	switch (ordersource)
	{
	case THOST_FTDC_OSRC_Participant:
		///���Բ�����
		return CW_FTDC_OSRC_Participant;
	case THOST_FTDC_OSRC_Administrator:
		///���Թ���Ա
		return CW_FTDC_OSRC_Administrator;
	default:
		break;
	}
	return cwFtdcOrderSourceType();
}

cwFtdcOrderStatusType cwFtdTradeSpiDemo::GetCtp2CwOrderStatusType(TThostFtdcOrderStatusType orderstatustype)
{
	switch (orderstatustype)
	{
	case THOST_FTDC_OST_AllTraded:
		///ȫ���ɽ�
		return CW_FTDC_OST_AllTraded;
	case THOST_FTDC_OST_PartTradedQueueing:
		///���ֳɽ����ڶ�����
		return CW_FTDC_OST_PartTradedQueueing;
	case THOST_FTDC_OST_PartTradedNotQueueing:
		///���ֳɽ����ڶ�����
		return CW_FTDC_OST_PartTradedNotQueueing;
	case THOST_FTDC_OST_NoTradeQueueing:
		///δ�ɽ����ڶ�����
		return CW_FTDC_OST_NoTradeQueueing;
	case THOST_FTDC_OST_NoTradeNotQueueing:
		///δ�ɽ����ڶ�����
		return CW_FTDC_OST_NoTradeNotQueueing;
	case THOST_FTDC_OST_Canceled:
		///����
		return CW_FTDC_OST_Canceled;
	case THOST_FTDC_OST_Unknown:
		///����
		return CW_FTDC_OST_Unknown;
	case THOST_FTDC_OST_NotTouched:
		///����
		return CW_FTDC_OST_NotTouched;
	case THOST_FTDC_OST_Touched:
		///����
		return CW_FTDC_OST_Touched;
	default:
		break;
	}
	return CW_FTDC_OST_cwDefault;
}

cwFtdcContingentConditionType cwFtdTradeSpiDemo::GetCtp2CwContingentConditionType(TThostFtdcContingentConditionType contingentconditiontype)
{
	switch (contingentconditiontype)
	{
	case THOST_FTDC_CC_Immediately:
		///����
		return CW_FTDC_CC_Immediately;
	case THOST_FTDC_CC_Touch:
		///ֹ��
		return CW_FTDC_CC_Touch;
	case THOST_FTDC_CC_TouchProfit:
		///ֹӮ
		return CW_FTDC_CC_TouchProfit;
	case THOST_FTDC_CC_ParkedOrder:
		///Ԥ��
		return CW_FTDC_CC_ParkedOrder;
	case THOST_FTDC_CC_LastPriceGreaterThanStopPrice:
		///���¼۴���������
		return CW_FTDC_CC_LastPriceGreaterThanStopPrice;
	case THOST_FTDC_CC_LastPriceGreaterEqualStopPrice:
		///���¼۴��ڵ���������
		return CW_FTDC_CC_LastPriceGreaterEqualStopPrice;
	case THOST_FTDC_CC_LastPriceLesserThanStopPrice:
		///���¼�С��������
		return CW_FTDC_CC_LastPriceLesserThanStopPrice;
	case THOST_FTDC_CC_LastPriceLesserEqualStopPrice:
		///���¼�С�ڵ���������
		return CW_FTDC_CC_LastPriceLesserEqualStopPrice;
	case THOST_FTDC_CC_AskPriceGreaterThanStopPrice:
		///��һ�۴���������
		return CW_FTDC_CC_AskPriceGreaterThanStopPrice;
	case THOST_FTDC_CC_AskPriceGreaterEqualStopPrice:
		///��һ�۴��ڵ���������
		return CW_FTDC_CC_AskPriceGreaterEqualStopPrice;
	case THOST_FTDC_CC_AskPriceLesserThanStopPrice:
		///��һ��С��������
		return CW_FTDC_CC_AskPriceLesserThanStopPrice;
	case THOST_FTDC_CC_AskPriceLesserEqualStopPrice:
		///��һ��С�ڵ���������
		return CW_FTDC_CC_AskPriceLesserEqualStopPrice;
	case THOST_FTDC_CC_BidPriceGreaterThanStopPrice:
		///��һ�۴���������
		return CW_FTDC_CC_BidPriceGreaterThanStopPrice;
	case THOST_FTDC_CC_BidPriceGreaterEqualStopPrice:
		///��һ�۴��ڵ���������
		return CW_FTDC_CC_BidPriceGreaterEqualStopPrice;
	case THOST_FTDC_CC_BidPriceLesserThanStopPrice:
		///��һ��С��������
		return CW_FTDC_CC_BidPriceLesserThanStopPrice;
	case THOST_FTDC_CC_BidPriceLesserEqualStopPrice:
		///��һ��С�ڵ���������
		return CW_FTDC_CC_BidPriceLesserEqualStopPrice;
	default:
		break;
	}
	return CW_FTDC_CC_Immediately;
}

cwFtdcOrderSubmitStatusType cwFtdTradeSpiDemo::GetCtp2CwOrderSubmitStatusType(TThostFtdcOrderSubmitStatusType ordersubmitstatustype)
{
	switch (ordersubmitstatustype)
	{
	case THOST_FTDC_OSS_InsertSubmitted:
		///�Ѿ��ύ
		return CW_FTDC_OSS_InsertSubmitted;
	case THOST_FTDC_OSS_CancelSubmitted:
		///�����Ѿ��ύ
		return CW_FTDC_OSS_CancelSubmitted;
	case THOST_FTDC_OSS_ModifySubmitted:
		///�޸��Ѿ��ύ
		return CW_FTDC_OSS_ModifySubmitted;
	case THOST_FTDC_OSS_Accepted:	
		///�Ѿ�����
		return CW_FTDC_OSS_Accepted;
	case THOST_FTDC_OSS_InsertRejected:
		///�����Ѿ����ܾ�
		return CW_FTDC_OSS_InsertRejected;
	case THOST_FTDC_OSS_CancelRejected:
		///�����Ѿ����ܾ�
		return CW_FTDC_OSS_CancelRejected;
	case THOST_FTDC_OSS_ModifyRejected:
		///�ĵ��Ѿ����ܾ�
		return CW_FTDC_OSS_ModifyRejected;
	default:
		break;
	}
	return cwFtdcOrderSubmitStatusType();
}

cwFtdcOrderTypeType cwFtdTradeSpiDemo::GetCtp2CwOrderTypetype(TThostFtdcOrderTypeType ordertype)
{
	switch (ordertype)
	{
	case THOST_FTDC_ORDT_Normal:
		///����
		return CW_FTDC_ORDT_Normal;
	case THOST_FTDC_ORDT_DeriveFromQuote:
		///��������
		return CW_FTDC_ORDT_DeriveFromQuote;
	case THOST_FTDC_ORDT_DeriveFromCombination:
		///�������
		return CW_FTDC_ORDT_DeriveFromCombination;
	case THOST_FTDC_ORDT_Combination:
		///��ϱ���
		return CW_FTDC_ORDT_Combination;
	case THOST_FTDC_ORDT_ConditionalOrder:
		///������
		return CW_FTDC_ORDT_ConditionalOrder;
	case THOST_FTDC_ORDT_Swap:
		///������
		return CW_FTDC_ORDT_Swap;
	default:
		break;
	}
	return CW_FTDC_ORDT_Normal;
}

cwFtdcTradeTypeType cwFtdTradeSpiDemo::GetCtp2CwTradeTypetype(TThostFtdcTradeTypeType tradetype)
{
	switch (tradetype)
	{
	case THOST_FTDC_TRDT_SplitCombination:
		///��ϳֲֲ��Ϊ��һ�ֲ�,��ʼ����Ӧ���������͵ĳֲ�
		return CW_FTDC_TRDT_SplitCombination;
	case THOST_FTDC_TRDT_Common:
		///��ͨ�ɽ�
		return CW_FTDC_TRDT_Common;
	case THOST_FTDC_TRDT_OptionsExecution:
		///��Ȩִ��
		return CW_FTDC_TRDT_OptionsExecution;
	case THOST_FTDC_TRDT_OTC:
		///OTC�ɽ�
		return CW_FTDC_TRDT_OTC;
	case THOST_FTDC_TRDT_EFPDerived:
		///��ת�������ɽ�
		return CW_FTDC_TRDT_EFPDerived;
	case THOST_FTDC_TRDT_CombinationDerived:
		///��������ɽ�
		return CW_FTDC_TRDT_CombinationDerived;
	default:
		break;
	}
	return CW_FTDC_TRDT_Common;
}

cwFtdcTradeSourceType cwFtdTradeSpiDemo::GetCtp2CwTradeSourceType(TThostFtdcTradeSourceType tradesourcetype)
{
	switch (tradesourcetype)
	{
	case THOST_FTDC_TSRC_NORMAL:
		///���Խ�������ͨ�ر�
		return CW_FTDC_TSRC_NORMAL;
	case THOST_FTDC_TSRC_QUERY:
		///���Բ�ѯ
		return CW_FTDC_TSRC_QUERY;
	default:
		break;
	}
	return CW_FTDC_TSRC_QUERY;
}

cwFtdcInstrumentStatusType cwFtdTradeSpiDemo::GetCtp2CwInstrumentStatusType(TThostFtdcInstrumentStatusType intrumentstatustype)
{
	switch (intrumentstatustype)
	{
	case THOST_FTDC_IS_BeforeTrading:
		///����ǰ
		return CW_FTDC_IS_BeforeTrading;
	case THOST_FTDC_IS_NoTrading:
		///�ǽ���
		return CW_FTDC_IS_NoTrading;
	case THOST_FTDC_IS_Continous:
		///��������
		return CW_FTDC_IS_Continous;
	case THOST_FTDC_IS_AuctionOrdering:
		///���Ͼ��۱���
		return CW_FTDC_IS_AuctionOrdering;
	case THOST_FTDC_IS_AuctionBalance:
		///���Ͼ��ۼ۸�ƽ��
		return CW_FTDC_IS_AuctionBalance;
	case THOST_FTDC_IS_AuctionMatch:
		///���Ͼ��۴��
		return CW_FTDC_IS_AuctionMatch;
	case THOST_FTDC_IS_Closed:
		///����
		return CW_FTDC_IS_Closed;
	default:
		break;
	}
	return cwFtdcInstrumentStatusType();
}

cwOrderPtr cwFtdTradeSpiDemo::GetcwOrderPtr(CThostFtdcOrderField * pOrder)
{
	cwOrderPtr OrderPtr(new ORDERFIELD());
	if (OrderPtr.get() == NULL
		|| pOrder == NULL)
	{
		OrderPtr.reset();
		return OrderPtr;
	}

	{
#ifdef _MSC_VER
		///���͹�˾����
		memcpy_s(OrderPtr->BrokerID, sizeof(OrderPtr->BrokerID), pOrder->BrokerID, sizeof(pOrder->BrokerID));
		///Ͷ���ߴ���
		memcpy_s(OrderPtr->InvestorID, sizeof(OrderPtr->InvestorID), pOrder->InvestorID, sizeof(pOrder->InvestorID));
		///��Լ����
		memcpy_s(OrderPtr->InstrumentID, sizeof(OrderPtr->InstrumentID), pOrder->InstrumentID, sizeof(pOrder->InstrumentID));
		///��������
		memcpy_s(OrderPtr->OrderRef, sizeof(OrderPtr->OrderRef), pOrder->OrderRef, sizeof(pOrder->OrderRef));
		///�û�����
		memcpy_s(OrderPtr->UserID, sizeof(OrderPtr->UserID), pOrder->UserID, sizeof(pOrder->UserID));
		///��������
		OrderPtr->Direction = GetCtp2CwDirectionType(pOrder->Direction);
		///��Ͽ�ƽ��־
		for (int x = 0; x < sizeof(pOrder->CombOffsetFlag)
			&& x < sizeof(OrderPtr->CombOffsetFlag); x++)
		{
			OrderPtr->CombOffsetFlag[x] = GetCtp2CwOffsetFlagType(pOrder->CombOffsetFlag[x]);
		}
		//memcpy_s(OrderPtr->CombOffsetFlag, sizeof(OrderPtr->CombOffsetFlag), pOrder->CombOffsetFlag, sizeof(pOrder->CombOffsetFlag));
		///���Ͷ���ױ���־
		for (int x = 0; x < sizeof(pOrder->CombHedgeFlag)
			&& x < sizeof(OrderPtr->CombHedgeFlag); x++)
		{
			OrderPtr->CombHedgeFlag[x] = GetCtp2CwHedgeFlagType(pOrder->CombHedgeFlag[x]);
		}
		//memcpy_s(OrderPtr->CombHedgeFlag, sizeof(OrderPtr->CombHedgeFlag), pOrder->CombHedgeFlag, sizeof(pOrder->CombHedgeFlag));
#else
		///���͹�˾����
		memcpy(OrderPtr->BrokerID, pOrder->BrokerID, sizeof(pOrder->BrokerID));
		///Ͷ���ߴ���
		memcpy(OrderPtr->InvestorID, pOrder->InvestorID, sizeof(pOrder->InvestorID));
		///��Լ����
		memcpy(OrderPtr->InstrumentID, pOrder->InstrumentID, sizeof(pOrder->InstrumentID));
		///��������
		memcpy(OrderPtr->OrderRef, pOrder->OrderRef, sizeof(pOrder->OrderRef));
		///�û�����
		memcpy(OrderPtr->UserID, pOrder->UserID, sizeof(pOrder->UserID));
		///��������
		OrderPtr->Direction = GetCtp2CwDirectionType(pOrder->Direction);
		///��Ͽ�ƽ��־
		for (int x = 0; x < (int)sizeof(pOrder->CombOffsetFlag)
			&& x < (int)sizeof(OrderPtr->CombOffsetFlag); x++)
		{
			OrderPtr->CombOffsetFlag[x] = GetCtp2CwOffsetFlagType(pOrder->CombOffsetFlag[x]);
		}
		//memcpy(OrderPtr->CombOffsetFlag, pOrder->CombOffsetFlag, sizeof(pOrder->CombOffsetFlag));
		///���Ͷ���ױ���־
		for (int x = 0; x < (int)sizeof(pOrder->CombHedgeFlag)
			&& x < (int)sizeof(OrderPtr->CombHedgeFlag); x++)
		{
			OrderPtr->CombHedgeFlag[x] = GetCtp2CwHedgeFlagType(pOrder->CombHedgeFlag[x]);
		}
		//memcpy(OrderPtr->CombHedgeFlag, pOrder->CombHedgeFlag, sizeof(pOrder->CombHedgeFlag));
#endif
			///�۸�
		OrderPtr->LimitPrice = pOrder->LimitPrice;
		///����
		OrderPtr->VolumeTotalOriginal = pOrder->VolumeTotalOriginal;
		///��С�ɽ���
		OrderPtr->MinVolume = pOrder->MinVolume;
		///�����۸�����
		OrderPtr->OrderPriceType = GetCtp2CwOrderPriceType(pOrder->OrderPriceType);
		///��Ч������
		OrderPtr->TimeCondition = GetCtp2CwTimeConditionType(pOrder->TimeCondition);
#ifdef _MSC_VER
		///GTD����
		memcpy_s(OrderPtr->GTDDate, sizeof(OrderPtr->GTDDate), pOrder->GTDDate, sizeof(pOrder->GTDDate));
#else
		///GTD����
		memcpy(OrderPtr->GTDDate, pOrder->GTDDate, sizeof(pOrder->GTDDate));
#endif
		///�ɽ�������
		OrderPtr->VolumeCondition = GetCtp2CwVolumeConditionType(pOrder->VolumeCondition);
		///��������
		OrderPtr->ContingentCondition = GetCtp2CwContingentConditionType(pOrder->ContingentCondition);
		///ǿƽԭ��
		OrderPtr->ForceCloseReason = GetCtd2CwForceCloseReasonType(pOrder->ForceCloseReason);
#ifdef _MSC_VER
		///���ر������
		memcpy_s(OrderPtr->OrderLocalID, sizeof(OrderPtr->OrderLocalID), pOrder->OrderLocalID, sizeof(pOrder->OrderLocalID));
		///����������
		memcpy_s(OrderPtr->ExchangeID, sizeof(OrderPtr->ExchangeID), pOrder->ExchangeID, sizeof(pOrder->ExchangeID));
		///�ͻ�����
		memcpy_s(OrderPtr->ClientID, sizeof(OrderPtr->ClientID), pOrder->ClientID, sizeof(pOrder->ClientID));
		///�����ύ״̬
		OrderPtr->OrderSubmitStatus = GetCtp2CwOrderSubmitStatusType(pOrder->OrderSubmitStatus);
		///ֹ���
		OrderPtr->StopPrice = pOrder->StopPrice;
		///������
		memcpy_s(OrderPtr->TradingDay, sizeof(OrderPtr->TradingDay), pOrder->TradingDay, sizeof(pOrder->TradingDay));
		///�������
		memcpy_s(OrderPtr->OrderSysID, sizeof(OrderPtr->OrderSysID), pOrder->OrderSysID, sizeof(pOrder->OrderSysID));

#else
		///���ر������
		memcpy(OrderPtr->OrderLocalID, pOrder->OrderLocalID, sizeof(pOrder->OrderLocalID));
		///����������
		memcpy(OrderPtr->ExchangeID, pOrder->ExchangeID, sizeof(pOrder->ExchangeID));
		///�ͻ�����
		memcpy(OrderPtr->ClientID, pOrder->ClientID, sizeof(pOrder->ClientID));
		///�����ύ״̬
		OrderPtr->OrderSubmitStatus = GetCtp2CwOrderSubmitStatusType(pOrder->OrderSubmitStatus);
		///ֹ���
		OrderPtr->StopPrice = pOrder->StopPrice;
		///������
		memcpy(OrderPtr->TradingDay, pOrder->TradingDay, sizeof(pOrder->TradingDay));
		///�������
		memcpy(OrderPtr->OrderSysID, pOrder->OrderSysID, sizeof(pOrder->OrderSysID));
#endif
		///������Դ
		OrderPtr->OrderSource = GetCtp2CwOrderSourceType(pOrder->OrderSource);
		///����״̬
		OrderPtr->OrderStatus = GetCtp2CwOrderStatusType(pOrder->OrderStatus);
		///��ɽ�����
		OrderPtr->VolumeTraded = pOrder->VolumeTraded;
		///ʣ������
		OrderPtr->VolumeTotal = pOrder->VolumeTotal;
#ifdef _MSC_VER
		///��������
		memcpy_s(OrderPtr->InsertDate, sizeof(OrderPtr->InsertDate), pOrder->InsertDate, sizeof(pOrder->InsertDate));
		///ί��ʱ��
		memcpy_s(OrderPtr->InsertTime, sizeof(OrderPtr->InsertTime), pOrder->InsertTime, sizeof(pOrder->InsertTime));
		///����ʱ��
		memcpy_s(OrderPtr->ActiveTime, sizeof(OrderPtr->ActiveTime), pOrder->ActiveTime, sizeof(pOrder->ActiveTime));
		///����ʱ��
		memcpy_s(OrderPtr->SuspendTime, sizeof(OrderPtr->SuspendTime), pOrder->SuspendTime, sizeof(pOrder->SuspendTime));
		///����޸�ʱ��
		memcpy_s(OrderPtr->UpdateTime, sizeof(OrderPtr->UpdateTime), pOrder->UpdateTime, sizeof(pOrder->UpdateTime));
		///����ʱ��
		memcpy_s(OrderPtr->CancelTime, sizeof(OrderPtr->CancelTime), pOrder->CancelTime, sizeof(pOrder->CancelTime));
		///�û��˲�Ʒ��Ϣ
		memcpy_s(OrderPtr->UserProductInfo, sizeof(OrderPtr->UserProductInfo), pOrder->UserProductInfo, sizeof(pOrder->UserProductInfo));
		///״̬��Ϣ
		memcpy_s(OrderPtr->StatusMsg, sizeof(OrderPtr->StatusMsg), pOrder->StatusMsg, sizeof(pOrder->StatusMsg));
		///��ر���
		memcpy_s(OrderPtr->RelativeOrderSysID, sizeof(OrderPtr->RelativeOrderSysID), pOrder->RelativeOrderSysID, sizeof(pOrder->RelativeOrderSysID));
#else
		///��������
		memcpy(OrderPtr->InsertDate, pOrder->InsertDate, sizeof(pOrder->InsertDate));
		///ί��ʱ��
		memcpy(OrderPtr->InsertTime, pOrder->InsertTime, sizeof(pOrder->InsertTime));
		///����ʱ��
		memcpy(OrderPtr->ActiveTime, pOrder->ActiveTime, sizeof(pOrder->ActiveTime));
		///����ʱ��
		memcpy(OrderPtr->SuspendTime, pOrder->SuspendTime, sizeof(pOrder->SuspendTime));
		///����޸�ʱ��
		memcpy(OrderPtr->UpdateTime, pOrder->UpdateTime, sizeof(pOrder->UpdateTime));
		///����ʱ��
		memcpy(OrderPtr->CancelTime, pOrder->CancelTime, sizeof(pOrder->CancelTime));
		///�û��˲�Ʒ��Ϣ
		memcpy(OrderPtr->UserProductInfo, pOrder->UserProductInfo, sizeof(pOrder->UserProductInfo));
		///״̬��Ϣ
		memcpy(OrderPtr->StatusMsg, pOrder->StatusMsg, sizeof(pOrder->StatusMsg));
		///��ر���
		memcpy(OrderPtr->RelativeOrderSysID, pOrder->RelativeOrderSysID, sizeof(pOrder->RelativeOrderSysID));
#endif
		///��������
		OrderPtr->OrderType = GetCtp2CwOrderTypetype(pOrder->OrderType);
		///������
		OrderPtr->SettlementID = pOrder->SettlementID;
		///ǰ�ñ��
		OrderPtr->FrontID = pOrder->FrontID;
		///�Ự���
		OrderPtr->SessionID = pOrder->SessionID;
		///�û�ǿ����־
		OrderPtr->UserForceClose = pOrder->UserForceClose;
		///Mac��ַ
		//cwFtdcMacAddressType				MacAddress;
#ifdef _MSC_VER
			///IP��ַ
		memcpy_s(OrderPtr->IPAddress, sizeof(OrderPtr->IPAddress), pOrder->IPAddress, sizeof(pOrder->IPAddress));
		///���ִ���
		memcpy_s(OrderPtr->CurrencyID, sizeof(OrderPtr->CurrencyID), pOrder->CurrencyID, sizeof(pOrder->CurrencyID));
#else
			///IP��ַ
		memcpy(OrderPtr->IPAddress, pOrder->IPAddress, sizeof(pOrder->IPAddress));
		///���ִ���
		memcpy(OrderPtr->CurrencyID, pOrder->CurrencyID, sizeof(pOrder->CurrencyID));
#endif
	}

	return OrderPtr;
}

cwOrderPtr cwFtdTradeSpiDemo::GetcwOrderPtr(CThostFtdcInputOrderField * pInputOrder)
{
	cwOrderPtr OrderPtr(new ORDERFIELD());
	if (OrderPtr.get() == NULL
		|| pInputOrder == NULL)
	{
		OrderPtr.reset();
		return OrderPtr;
	}

	{
#ifdef _MSC_VER
		///���͹�˾����
		memcpy_s(OrderPtr->BrokerID, sizeof(OrderPtr->BrokerID), pInputOrder->BrokerID, sizeof(pInputOrder->BrokerID));
		///Ͷ���ߴ���
		memcpy_s(OrderPtr->InvestorID, sizeof(OrderPtr->InvestorID), pInputOrder->InvestorID, sizeof(pInputOrder->InvestorID));
		///��Լ����
		memcpy_s(OrderPtr->InstrumentID, sizeof(OrderPtr->InstrumentID), pInputOrder->InstrumentID, sizeof(pInputOrder->InstrumentID));
		///��������
		memcpy_s(OrderPtr->OrderRef, sizeof(OrderPtr->OrderRef), pInputOrder->OrderRef, sizeof(pInputOrder->OrderRef));
		///�û�����
		memcpy_s(OrderPtr->UserID, sizeof(OrderPtr->UserID), pInputOrder->UserID, sizeof(pInputOrder->UserID));
		///�����۸�����
		OrderPtr->OrderPriceType = GetCtp2CwOrderPriceType(pInputOrder->OrderPriceType);
		///��������
		OrderPtr->Direction = GetCtp2CwDirectionType(pInputOrder->Direction);
		///��Ͽ�ƽ��־
		memcpy_s(OrderPtr->CombOffsetFlag, sizeof(OrderPtr->CombOffsetFlag), pInputOrder->CombOffsetFlag, sizeof(pInputOrder->CombOffsetFlag));
		///���Ͷ���ױ���־
		memcpy_s(OrderPtr->CombHedgeFlag, sizeof(OrderPtr->CombHedgeFlag), pInputOrder->CombHedgeFlag, sizeof(pInputOrder->CombHedgeFlag));
		///�۸�
		OrderPtr->LimitPrice = pInputOrder->LimitPrice;
		///����
		OrderPtr->VolumeTotalOriginal = pInputOrder->VolumeTotalOriginal;
		///��ɽ�����
		OrderPtr->VolumeTraded = 0;
		///ʣ������
		OrderPtr->VolumeTotal = pInputOrder->VolumeTotalOriginal;
		///��Ч������
		OrderPtr->TimeCondition = GetCtp2CwTimeConditionType(pInputOrder->TimeCondition);
		///GTD����
		memcpy_s(OrderPtr->GTDDate, sizeof(OrderPtr->GTDDate), pInputOrder->GTDDate, sizeof(pInputOrder->GTDDate));
		///�ɽ�������
		OrderPtr->VolumeCondition = GetCtp2CwVolumeConditionType(pInputOrder->VolumeCondition);
		///��С�ɽ���
		OrderPtr->MinVolume = pInputOrder->MinVolume;
		///��������
		OrderPtr->ContingentCondition = GetCtp2CwContingentConditionType(pInputOrder->ContingentCondition);
		///ֹ���
		OrderPtr->StopPrice = pInputOrder->StopPrice;
		///ǿƽԭ��
		OrderPtr->ForceCloseReason = GetCtd2CwForceCloseReasonType(pInputOrder->ForceCloseReason);
		///�Զ������־
		//OrderPtr->IsAutoSuspend = pInputOrder->IsAutoSuspend;
		///ҵ��Ԫ
		//memcpy_s(OrderPtr->BusinessUnit, sizeof(OrderPtr->BusinessUnit), pInputOrder->BusinessUnit, sizeof(pInputOrder->BusinessUnit));
		///������
		//OrderPtr->RequestID = pInputOrder->RequestID;
#ifdef _MSC_VER
#pragma region NoOrderInputField
#endif
		///���ر������
		//TThostFtdcOrderLocalIDType	OrderLocalID;
		///����������
		//TThostFtdcExchangeIDType	ExchangeID;
		///��Ա����
		//TThostFtdcParticipantIDType	ParticipantID;
		///�ͻ�����
		//TThostFtdcClientIDType	ClientID;
		///��Լ�ڽ������Ĵ���
		//TThostFtdcExchangeInstIDType	ExchangeInstID;
		///����������Ա����
		//TThostFtdcTraderIDType	TraderID;
		///��װ���
		//TThostFtdcInstallIDType	InstallID;
		///�����ύ״̬
		//TThostFtdcOrderSubmitStatusType	OrderSubmitStatus;
		///������ʾ���
		//TThostFtdcSequenceNoType	NotifySequence;
		///������
		//TThostFtdcDateType	TradingDay;
		///������
		//TThostFtdcSettlementIDType	SettlementID;
		///�������
		//TThostFtdcOrderSysIDType	OrderSysID;
		///������Դ
		//TThostFtdcOrderSourceType	OrderSource;
		///����״̬
		//TThostFtdcOrderStatusType	OrderStatus;
		///��������
		//TThostFtdcOrderTypeType	OrderType;
		///��������
		//TThostFtdcDateType	InsertDate;
		///ί��ʱ��
		//TThostFtdcTimeType	InsertTime;
		///����ʱ��
		//TThostFtdcTimeType	ActiveTime;
		///����ʱ��
		//TThostFtdcTimeType	SuspendTime;
		///����޸�ʱ��
		//TThostFtdcTimeType	UpdateTime;
		///����ʱ��
		//TThostFtdcTimeType	CancelTime;
		///����޸Ľ���������Ա����
		//TThostFtdcTraderIDType	ActiveTraderID;
		///�����Ա���
		//TThostFtdcParticipantIDType	ClearingPartID;
		///���
		//TThostFtdcSequenceNoType	SequenceNo;
		///ǰ�ñ��
		//TThostFtdcFrontIDType	FrontID;
		///�Ự���
		//TThostFtdcSessionIDType	SessionID;
		///�û��˲�Ʒ��Ϣ
		//TThostFtdcProductInfoType	UserProductInfo;
		///״̬��Ϣ
		//TThostFtdcErrorMsgType	StatusMsg;
		///�����û�����
		//TThostFtdcUserIDType	ActiveUserID;
		///���͹�˾�������
		//TThostFtdcSequenceNoType	BrokerOrderSeq;
		///��ر���
		//TThostFtdcOrderSysIDType	RelativeOrderSysID;
		///֣�����ɽ�����
		//TThostFtdcVolumeType	ZCETotalTradedVolume;
#ifdef _MSC_VER
#pragma endregion
#endif
	///�û�ǿ����־
		OrderPtr->UserForceClose = pInputOrder->UserForceClose;
		///��������־
		//OrderPtr->IsSwapOrder = pInputOrder->IsSwapOrder;
		///Ӫҵ�����
		//TThostFtdcBranchIDType	BranchID;
		///Ͷ�ʵ�Ԫ����
		//memcpy_s(OrderPtr->InvestUnitID, sizeof(OrderPtr->InvestUnitID), pInputOrder->InvestUnitID, sizeof(pInputOrder->InvestUnitID));
		///�ʽ��˺�
		//memcpy_s(OrderPtr->AccountID, sizeof(OrderPtr->AccountID), pInputOrder->AccountID, sizeof(pInputOrder->AccountID));
		///���ִ���
		memcpy_s(OrderPtr->CurrencyID, sizeof(OrderPtr->CurrencyID), pInputOrder->CurrencyID, sizeof(pInputOrder->CurrencyID));
		///IP��ַ
		memcpy_s(OrderPtr->IPAddress, sizeof(OrderPtr->IPAddress), pInputOrder->IPAddress, sizeof(pInputOrder->IPAddress));
		///Mac��ַ
		//memcpy_s(OrderPtr->MacAddress, sizeof(OrderPtr->MacAddress), pInputOrder->MacAddress, sizeof(pInputOrder->MacAddress));
#else
		///���͹�˾����
		memcpy(OrderPtr->BrokerID, pInputOrder->BrokerID, sizeof(pInputOrder->BrokerID));
		///Ͷ���ߴ���
		memcpy(OrderPtr->InvestorID, pInputOrder->InvestorID, sizeof(pInputOrder->InvestorID));
		///��Լ����
		memcpy(OrderPtr->InstrumentID, pInputOrder->InstrumentID, sizeof(pInputOrder->InstrumentID));
		///��������
		memcpy(OrderPtr->OrderRef, pInputOrder->OrderRef, sizeof(pInputOrder->OrderRef));
		///�û�����
		memcpy(OrderPtr->UserID, pInputOrder->UserID, sizeof(pInputOrder->UserID));
		///�����۸�����
		OrderPtr->OrderPriceType = GetCtp2CwOrderPriceType(pInputOrder->OrderPriceType);
		///��������
		OrderPtr->Direction = GetCtp2CwDirectionType(pInputOrder->Direction);
		///��Ͽ�ƽ��־
		memcpy(OrderPtr->CombOffsetFlag, pInputOrder->CombOffsetFlag, sizeof(pInputOrder->CombOffsetFlag));
		///���Ͷ���ױ���־
		memcpy(OrderPtr->CombHedgeFlag, pInputOrder->CombHedgeFlag, sizeof(pInputOrder->CombHedgeFlag));
		///�۸�
		OrderPtr->LimitPrice = pInputOrder->LimitPrice;
		///����
		OrderPtr->VolumeTotalOriginal = pInputOrder->VolumeTotalOriginal;
		///��ɽ�����
		OrderPtr->VolumeTraded = 0;
		///ʣ������
		OrderPtr->VolumeTotal = pInputOrder->VolumeTotalOriginal;
		///��Ч������
		OrderPtr->TimeCondition = GetCtp2CwTimeConditionType(pInputOrder->TimeCondition);
		///GTD����
		memcpy(OrderPtr->GTDDate, pInputOrder->GTDDate, sizeof(pInputOrder->GTDDate));
		///�ɽ�������
		OrderPtr->VolumeCondition = GetCtp2CwVolumeConditionType(pInputOrder->VolumeCondition);
		///��С�ɽ���
		OrderPtr->MinVolume = pInputOrder->MinVolume;
		///��������
		OrderPtr->ContingentCondition = GetCtp2CwContingentConditionType(pInputOrder->ContingentCondition);
		///ֹ���
		OrderPtr->StopPrice = pInputOrder->StopPrice;
		///ǿƽԭ��
		OrderPtr->ForceCloseReason = GetCtd2CwForceCloseReasonType(pInputOrder->ForceCloseReason);
		///�Զ������־
		//OrderPtr->IsAutoSuspend = pInputOrder->IsAutoSuspend;
		///ҵ��Ԫ
		//memcpy(OrderPtr->BusinessUnit, pInputOrder->BusinessUnit, sizeof(pInputOrder->BusinessUnit));
		///������
		//OrderPtr->RequestID = pInputOrder->RequestID;
#ifdef _MSC_VER
#pragma region NoOrderInputField
#endif
		///���ر������
		//TThostFtdcOrderLocalIDType	OrderLocalID;
		///����������
		//TThostFtdcExchangeIDType	ExchangeID;
		///��Ա����
		//TThostFtdcParticipantIDType	ParticipantID;
		///�ͻ�����
		//TThostFtdcClientIDType	ClientID;
		///��Լ�ڽ������Ĵ���
		//TThostFtdcExchangeInstIDType	ExchangeInstID;
		///����������Ա����
		//TThostFtdcTraderIDType	TraderID;
		///��װ���
		//TThostFtdcInstallIDType	InstallID;
		///�����ύ״̬
		//TThostFtdcOrderSubmitStatusType	OrderSubmitStatus;
		///������ʾ���
		//TThostFtdcSequenceNoType	NotifySequence;
		///������
		//TThostFtdcDateType	TradingDay;
		///������
		//TThostFtdcSettlementIDType	SettlementID;
		///�������
		//TThostFtdcOrderSysIDType	OrderSysID;
		///������Դ
		//TThostFtdcOrderSourceType	OrderSource;
		///����״̬
		//TThostFtdcOrderStatusType	OrderStatus;
		///��������
		//TThostFtdcOrderTypeType	OrderType;
		///��������
		//TThostFtdcDateType	InsertDate;
		///ί��ʱ��
		//TThostFtdcTimeType	InsertTime;
		///����ʱ��
		//TThostFtdcTimeType	ActiveTime;
		///����ʱ��
		//TThostFtdcTimeType	SuspendTime;
		///����޸�ʱ��
		//TThostFtdcTimeType	UpdateTime;
		///����ʱ��
		//TThostFtdcTimeType	CancelTime;
		///����޸Ľ���������Ա����
		//TThostFtdcTraderIDType	ActiveTraderID;
		///�����Ա���
		//TThostFtdcParticipantIDType	ClearingPartID;
		///���
		//TThostFtdcSequenceNoType	SequenceNo;
		///ǰ�ñ��
		//TThostFtdcFrontIDType	FrontID;
		///�Ự���
		//TThostFtdcSessionIDType	SessionID;
		///�û��˲�Ʒ��Ϣ
		//TThostFtdcProductInfoType	UserProductInfo;
		///״̬��Ϣ
		//TThostFtdcErrorMsgType	StatusMsg;
		///�����û�����
		//TThostFtdcUserIDType	ActiveUserID;
		///���͹�˾�������
		//TThostFtdcSequenceNoType	BrokerOrderSeq;
		///��ر���
		//TThostFtdcOrderSysIDType	RelativeOrderSysID;
		///֣�����ɽ�����
		//TThostFtdcVolumeType	ZCETotalTradedVolume;
#ifdef _MSC_VER
#pragma endregion
#endif
///�û�ǿ����־
		OrderPtr->UserForceClose = pInputOrder->UserForceClose;
		///��������־
		//OrderPtr->IsSwapOrder = pInputOrder->IsSwapOrder;
		///Ӫҵ�����
		//TThostFtdcBranchIDType	BranchID;
		///Ͷ�ʵ�Ԫ����
		//memcpy(OrderPtr->InvestUnitID, pInputOrder->InvestUnitID, sizeof(pInputOrder->InvestUnitID));
		///�ʽ��˺�
		//memcpy(OrderPtr->AccountID, pInputOrder->AccountID, sizeof(pInputOrder->AccountID));
		///���ִ���
		memcpy(OrderPtr->CurrencyID, pInputOrder->CurrencyID, sizeof(pInputOrder->CurrencyID));
		///IP��ַ
		memcpy(OrderPtr->IPAddress, pInputOrder->IPAddress, sizeof(pInputOrder->IPAddress));
		///Mac��ַ
		//memcpy(OrderPtr->MacAddress, pInputOrder->MacAddress, sizeof(pInputOrder->MacAddress));
#endif
	}
	return OrderPtr;

}

cwTradePtr cwFtdTradeSpiDemo::GetcwTradePtr(CThostFtdcTradeField * pTrade)
{
	cwTradePtr TradePtr(new TRADEFIELD());
	if (TradePtr.get() == NULL
		|| pTrade == NULL)
	{
		TradePtr.reset();
		return TradePtr;
	}

	{
#ifdef _MSC_VER
		memcpy_s(TradePtr->BrokerID, sizeof(TradePtr->BrokerID), pTrade->BrokerID, sizeof(pTrade->BrokerID));
		memcpy_s(TradePtr->InvestorID, sizeof(TradePtr->InvestorID), pTrade->InvestorID, sizeof(pTrade->InvestorID));
		memcpy_s(TradePtr->InstrumentID, sizeof(TradePtr->InstrumentID), pTrade->InstrumentID, sizeof(pTrade->InstrumentID));
		memcpy_s(TradePtr->OrderRef, sizeof(TradePtr->OrderRef), pTrade->OrderRef, sizeof(pTrade->OrderRef));
		memcpy_s(TradePtr->UserID, sizeof(TradePtr->UserID), pTrade->UserID, sizeof(pTrade->UserID));
		memcpy_s(TradePtr->TradeID, sizeof(TradePtr->TradeID), pTrade->TradeID, sizeof(pTrade->TradeID));
		TradePtr->Direction = GetCtp2CwDirectionType(pTrade->Direction);
		memcpy_s(TradePtr->OrderSysID, sizeof(TradePtr->OrderSysID), pTrade->OrderSysID, sizeof(pTrade->OrderSysID));
		memcpy_s(TradePtr->ClientID, sizeof(TradePtr->ClientID), pTrade->ClientID, sizeof(pTrade->ClientID));
		TradePtr->OffsetFlag = GetCtp2CwOffsetFlagType(pTrade->OffsetFlag);
		TradePtr->HedgeFlag = GetCtp2CwHedgeFlagType(pTrade->HedgeFlag);
		memcpy_s(TradePtr->TradeDate, sizeof(TradePtr->TradeDate), pTrade->TradeDate, sizeof(pTrade->TradeDate));
		memcpy_s(TradePtr->TradeTime, sizeof(TradePtr->TradeTime), pTrade->TradeTime, sizeof(pTrade->TradeTime));
		TradePtr->Price = pTrade->Price;
		TradePtr->Volume = pTrade->Volume;
		TradePtr->TradeType = GetCtp2CwTradeTypetype(pTrade->TradeType);
		TradePtr->TradeSource = GetCtp2CwTradeSourceType(pTrade->TradeSource);
		memcpy_s(TradePtr->TraderID, sizeof(TradePtr->TraderID), pTrade->TraderID, sizeof(pTrade->TraderID));
		memcpy_s(TradePtr->OrderLocalID, sizeof(TradePtr->OrderLocalID), pTrade->OrderLocalID, sizeof(pTrade->OrderLocalID));
#else
		memcpy(TradePtr->BrokerID, pTrade->BrokerID, sizeof(pTrade->BrokerID));
		memcpy(TradePtr->InvestorID, pTrade->InvestorID, sizeof(pTrade->InvestorID));
		memcpy(TradePtr->InstrumentID, pTrade->InstrumentID, sizeof(pTrade->InstrumentID));
		memcpy(TradePtr->OrderRef, pTrade->OrderRef, sizeof(pTrade->OrderRef));
		memcpy(TradePtr->UserID, pTrade->UserID, sizeof(pTrade->UserID));
		memcpy(TradePtr->TradeID, pTrade->TradeID, sizeof(pTrade->TradeID));
		TradePtr->Direction = GetCtp2CwDirectionType(pTrade->Direction);
		memcpy(TradePtr->OrderSysID, pTrade->OrderSysID, sizeof(pTrade->OrderSysID));
		memcpy(TradePtr->ClientID, pTrade->ClientID, sizeof(pTrade->ClientID));
		TradePtr->OffsetFlag = GetCtp2CwOffsetFlagType(pTrade->OffsetFlag);
		TradePtr->HedgeFlag = GetCtp2CwHedgeFlagType(pTrade->HedgeFlag);
		memcpy(TradePtr->TradeDate, pTrade->TradeDate, sizeof(pTrade->TradeDate));
		memcpy(TradePtr->TradeTime, pTrade->TradeTime, sizeof(pTrade->TradeTime));
		TradePtr->Price = pTrade->Price;
		TradePtr->Volume = pTrade->Volume;
		TradePtr->TradeType = GetCtp2CwTradeTypetype(pTrade->TradeType);
		TradePtr->TradeSource = GetCtp2CwTradeSourceType(pTrade->TradeSource);
		memcpy(TradePtr->TraderID, pTrade->TraderID, sizeof(pTrade->TraderID));
		memcpy(TradePtr->OrderLocalID, pTrade->OrderLocalID, sizeof(pTrade->OrderLocalID));
#endif
	}

	return TradePtr;
}

#ifdef _MSC_VER
#pragma endregion
#endif // _MSC_VER

void cwFtdTradeSpiDemo::LoopReqQryThread()
{
	uint64_t iCnt = 0;

	cwReqType nType;
	bool	  bNeedToReq = false;
	while (m_bReqQryThreadRun 
		&& m_CurrentStatus != TradeServerStatus::Status_UnConnected
		&& m_pTraderUserApi != NULL)
	{
		bNeedToReq = false;
		m_PrioritizedReqListMutex.lock();
		if (m_PrioritizedReqList.size() > 0)
		{
			bNeedToReq = true;
			nType = m_PrioritizedReqList.front();
			m_PrioritizedReqList.pop_front();
		}
		m_PrioritizedReqListMutex.unlock();

		if (bNeedToReq)
		{
			switch (nType)
			{
			case cwReqType::cwReqQryTradingAccount:
			{
				CThostFtdcQryTradingAccountField cwQryFild;
#ifdef _MSC_VER
				ZeroMemory(&cwQryFild, sizeof(CThostFtdcQryTradingAccountField));
				strncpy_s(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
				strncpy_s(cwQryFild.InvestorID, m_strInvestorID.c_str(), m_strInvestorID.size());
#else
				memset(&cwQryFild, 0, sizeof(CThostFtdcQryTradingAccountField));
				strncpy(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
				strncpy(cwQryFild.InvestorID, m_strInvestorID.c_str(), m_strInvestorID.size());
#endif // _MSC_VER
				MyReqFunction(cwReqQryTradingAccount, (void *)(&cwQryFild));
			}
			break;
			case cwReqType::cwRspQryInvestorPosition:
			{
				//CThostFtdcQryInvestorPositionField cwQryFild;
				//ZeroMemory(&cwQryFild, sizeof(CThostFtdcQryInvestorPositionField));
				//strncpy_s(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
				//strncpy_s(cwQryFild.InvestorID, m_strInvestorID.c_str(), m_strInvestorID.size());

				//MyReqFunction(cwRspQryInvestorPosition, (void *)(&cwQryFild));
			}
			break;
			default:
				break;
			}
		}

		if (iCnt % 45 == 0)
		{
			CThostFtdcQryTradingAccountField cwQryFild;
#ifdef _MSC_VER
			ZeroMemory(&cwQryFild, sizeof(CThostFtdcQryTradingAccountField));
			strncpy_s(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
			strncpy_s(cwQryFild.InvestorID, m_strInvestorID.c_str(), m_strInvestorID.size());
#else
			memset(&cwQryFild, 0, sizeof(CThostFtdcQryTradingAccountField));
			strncpy(cwQryFild.BrokerID, m_ReqUserLoginField.BrokerID, sizeof(cwQryFild.BrokerID));
			strncpy(cwQryFild.InvestorID, m_strInvestorID.c_str(), m_strInvestorID.size());
#endif // _MSC_VER
			m_pTraderUserApi->ReqQryTradingAccount(&cwQryFild, m_iRequestId++);
		}
		cwSleep(100);
		iCnt++;
	}
}

THOST_TE_RESUME_TYPE cwFtdTradeSpiDemo::Getcw2CtpResumeType(CW_TE_RESUME_TYPE type)
{
	switch (type)
	{
	case CW_TERT_RESTART:
		return THOST_TERT_QUICK;
	case CW_TERT_RESUME:
		return THOST_TERT_RESUME;
	case CW_TERT_QUICK:
		return THOST_TERT_RESTART;
	default:
		break;
	}
	return THOST_TERT_RESTART;
}
