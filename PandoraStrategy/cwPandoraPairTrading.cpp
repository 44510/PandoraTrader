#include "cwPandoraPairTrading.h"
#include <algorithm>


cwPandoraPairTrading::cwPandoraPairTrading()
{
	m_MainInstrumentID = "au2012";
	m_SubMainInstrumentID = "au2010";

	m_dBuyThreadHold = 1.6;
	m_dSellThreadHold = 1.72;

	m_dVolumeCoefficient = -1;

	m_cwMainOpenCloseMode = CloseTodayThenYd;
	m_cwSubMainOpenCloseMode = CloseTodayThenYd;

	m_iPositionLimit = 1;
	m_iOrderVolume = 1;
}


cwPandoraPairTrading::~cwPandoraPairTrading()
{
}

void cwPandoraPairTrading::PriceUpdate(cwMarketDataPtr pPriceData)
{
	if (pPriceData.get() == NULL)
	{
		return;
	}

	m_strCurrentUpdateTime = pPriceData->UpdateTime;

	//������������
	if (m_MainInstrumentID == (std::string)pPriceData->InstrumentID)
	{
		m_cwMainMarketData = pPriceData;
	}

	if (m_SubMainInstrumentID == (std::string)pPriceData->InstrumentID)
	{
		m_cwSubMainMarketData = pPriceData;
	}

	//ȷ�����������Ƿ��Ѿ�����Ч
	if (m_cwSubMainMarketData.get() == NULL
		|| m_cwMainMarketData.get() == NULL)
	{
		return;
	}

	//ȷ����ʼ�����
	if ((!m_bStrategyReady))
	{
		return;
	}

	DoManualSpread();

	if (m_pPositionAgent.get() != NULL
		&& m_pPositionAgent->pPositionAgent.get() != NULL)
	{
		m_pPositionAgent->pPositionAgent->SetExpectPosition(-1 * GetNetPosition(m_SubMainInstrumentID));
	}
	
}

void cwPandoraPairTrading::OnReady()
{
	SetAgentManager(dynamic_cast<cwAgentManager*>(&m_PandoraAgentManager));
	m_pPositionAgent = m_PandoraAgentManager.RegisterAgent(m_MainInstrumentID, cwPandoraAgentManager::Enum_Agent_Postion);
	if (m_pPositionAgent.get() != NULL
		&& m_pPositionAgent->pPositionAgent.get() != NULL)
	{
		//�����㷨����
		m_pPositionAgent->pPositionAgent->InsLargeOrderVolume = 100;
		m_pPositionAgent->pPositionAgent->InsLittleOrderVolume = 5;
		m_pPositionAgent->pPositionAgent->InsAskBidGap = 1;

		m_pPositionAgent->pPositionAgent->SetExpectPosition(-1 * GetNetPosition(m_SubMainInstrumentID));
	}

	//��������
	std::vector<std::string> SubscribeInstrument;

	SubscribeInstrument.push_back(m_MainInstrumentID);
	SubscribeInstrument.push_back(m_SubMainInstrumentID);

	SubScribePrice(SubscribeInstrument);
}

void cwPandoraPairTrading::DoManualSpread()
{
	cwEasyStrategyLog log(m_StrategyLog, "DoManualSpread");

	bool bStrategyCanOpen = true;

	//��ȡ�����ʹ�������Լ����С�䶯
	double dMainTickSize = GetTickSize(m_MainInstrumentID.c_str());
	if (dMainTickSize < 0)
	{
		return;
	}
	double dSubMainTickSize = GetTickSize(m_SubMainInstrumentID.c_str());
	if (dSubMainTickSize < 0)
	{
		return;
	}

	//��ȡ��������
	int iSubMainCancelCount = GetInstrumentCancelCount(m_SubMainInstrumentID);
	//������Ҫ�����double����
	const double dInsEQ = (double)(std::min)(dMainTickSize, dSubMainTickSize) / 10.0;


	//ÿ������ʱ�ο�����һС��ʱ�䲻����
	{
		cwProductTradeTime::cwTradeTimeSpace TradeTimeSpace;
		int iOpen = 0, iClose = 0;

		bool bRet = GetTradeTimeSpace(m_SubMainInstrumentID.c_str(), m_strCurrentUpdateTime.c_str(),
			TradeTimeSpace, iOpen, iClose);

		if (!bRet)
		{
			bStrategyCanOpen = false;
		}
		else
		{
			switch (TradeTimeSpace)
			{
			case cwProductTradeTime::NoTrading:
				bStrategyCanOpen = false;
				break;
			case cwProductTradeTime::AMPartOne:
				if (iOpen < 1
					|| iClose < 5)
				{
					bStrategyCanOpen = false;
				}
				break;
			case cwProductTradeTime::AMPartTwo:
				if (iOpen < 1
					|| iClose < 5)
				{
					bStrategyCanOpen = false;
				}
				break;
			case cwProductTradeTime::PMPartOne:

				if (iOpen < 1)
				{
					bStrategyCanOpen = false;
				}
				break;
			case cwProductTradeTime::NightPartOne:
				if (iOpen < 1)
				{
					bStrategyCanOpen = false;
				}
				break;

			default:
				break;
			}
		}

	}

	if (!bStrategyCanOpen)
	{
		std::map<cwActiveOrderKey, cwOrderPtr> WaitOrderList;
		GetActiveOrders(WaitOrderList);

		for (auto WaitOrderIt = WaitOrderList.begin();
			WaitOrderIt != WaitOrderList.end(); WaitOrderIt++)
		{
			if (m_SubMainInstrumentID == (std::string)WaitOrderIt->second->InstrumentID)
			{
				CancelOrder(WaitOrderIt->second);
			}
		}
		return;
	}

	std::map<std::string, cwPositionPtr> CurrentPosMap;
	std::map<cwActiveOrderKey, cwOrderPtr> WaitOrderList;
	GetPositionsAndActiveOrders(CurrentPosMap, WaitOrderList);

	int iMainPosition = 0, iSubMainPosition = 0;
	auto PosIt = CurrentPosMap.find(m_MainInstrumentID);
	if (PosIt != CurrentPosMap.end())
	{
		iMainPosition = PosIt->second->GetLongTotalPosition() - PosIt->second->GetShortTotalPosition();
	}
	else
	{
		iMainPosition = 0;
	}

	log.AddLog(cwStrategyLog::enMsg, "MainIns:%s Maintain:%d", m_MainInstrumentID.c_str(), iMainPosition);

	PosIt = CurrentPosMap.find(m_SubMainInstrumentID);
	if (PosIt != CurrentPosMap.end())
	{
		iSubMainPosition = PosIt->second->GetLongTotalPosition() - PosIt->second->GetShortTotalPosition();
	}
	else
	{
		iSubMainPosition = 0;
	}

	log.AddLog(cwStrategyLog::enMsg, "SubIns:%s SubMaintain:%d", m_SubMainInstrumentID.c_str(), iSubMainPosition);

	if (m_cwMainMarketData->UpperLimitPrice - m_cwMainMarketData->BidPrice1 < 3 * dMainTickSize + dInsEQ
		|| m_cwMainMarketData->AskPrice1 - m_cwMainMarketData->LowerLimitPrice < 3 * dMainTickSize + dInsEQ)
	{
		return;
	}
	if (m_cwSubMainMarketData->UpperLimitPrice - m_cwSubMainMarketData->BidPrice1 < 3 * dSubMainTickSize + dInsEQ
		|| m_cwSubMainMarketData->AskPrice1 - m_cwSubMainMarketData->LowerLimitPrice < 3 * dSubMainTickSize + dInsEQ)
	{
		return;
	}
	double dSubMainAskBidGap = m_cwSubMainMarketData->AskPrice1 - m_cwSubMainMarketData->BidPrice1;

	log.AddLog(cwStrategyLog::enMsg, "MainBid1:%.2f, SubMainBid1:%.2f", m_cwMainMarketData->BidPrice1, m_cwSubMainMarketData->BidPrice1);
	log.AddLog(cwStrategyLog::enMsg, "MainAsk1:%.2f, SubMainAsk1:%.2f", m_cwMainMarketData->AskPrice1, m_cwSubMainMarketData->AskPrice1);

#ifdef _MSC_VER
#pragma region BearSpread
#endif
	///��������������Զ�ں�Լ����ǰ�۲���ھ�ֵ
	if (iSubMainPosition< 0)
	{
		///��ǰ���ֲ�Ϊ�ղ֣���ƽ��, ��ƽ�ֲ�����ƽ���������ڿ�������

		bool bNeedCancel = false;
		bool bCanOpen = false;
		//�ȼ��ҵ�
		int iSubMainWaitLongOrder = 0;
		for (auto WaitOrderIt = WaitOrderList.begin();
			WaitOrderIt != WaitOrderList.end(); WaitOrderIt++)
		{
			bNeedCancel = true;
			if (m_SubMainInstrumentID == (std::string)WaitOrderIt->second->InstrumentID
				&& CW_FTDC_D_Buy == WaitOrderIt->second->Direction)
			{
				if (bNeedCancel
					&& m_cwMainMarketData->BidPrice1 - WaitOrderIt->second->LimitPrice > m_dSellThreadHold - dInsEQ)
				{
					bNeedCancel &= false;
				}

				//
				if (bNeedCancel
					|| m_cwSubMainMarketData->BidPrice1 - WaitOrderIt->second->LimitPrice > 2 * dSubMainTickSize - dInsEQ)
				{
					CancelOrder(WaitOrderIt->second);
					log.AddLog(cwStrategyLog::enCO, "%s, Ref:%s, P:%.2f, V:%d %s", WaitOrderIt->second->InstrumentID, WaitOrderIt->second->OrderRef,
						WaitOrderIt->second->LimitPrice, WaitOrderIt->second->VolumeTotal, WaitOrderIt->second->Direction == CW_FTDC_D_Buy ? "B" : "S");
				}
				iSubMainWaitLongOrder += WaitOrderIt->second->VolumeTotal;
			}
		}

		//��ƽ��
		if (bCanOpen
			|| m_cwMainMarketData->BidPrice1 - m_cwSubMainMarketData->AskPrice1 > m_dSellThreadHold - dInsEQ)
		{
			bCanOpen |= true;
		}

		if (bCanOpen
			&& iSubMainWaitLongOrder == 0)
		{
			double dbOrderPrice = m_cwSubMainMarketData->AskPrice1;

			int iVol = - (iSubMainWaitLongOrder + iSubMainPosition);
			if (iVol > m_iOrderVolume)
			{
				iVol = m_iOrderVolume;
			}

			cwOrderPtr orderptr = EasyInputOrder(m_SubMainInstrumentID.c_str(), iVol, dbOrderPrice, m_cwSubMainOpenCloseMode);
			if (orderptr.get() != NULL)
			{
				log.AddLog(cwStrategyLog::enIO, "%s, Ref:%s, P:%.2f, V:%d %s", orderptr->InstrumentID, orderptr->OrderRef,
					orderptr->LimitPrice, orderptr->VolumeTotal, orderptr->Direction == CW_FTDC_D_Buy ? "B" : "S");
			}
		}
	}
	else
	{
		///�ÿ��ֲ���
		bool bNeedCancel = true;
		bool bCanOpen = false;
		//�ȼ��ҵ�
		int iSubMainWaitLongOrder = 0;
		for (auto WaitOrderIt = WaitOrderList.begin();
			WaitOrderIt != WaitOrderList.end(); WaitOrderIt++)
		{
			bNeedCancel = true;
			if (m_SubMainInstrumentID == (std::string)WaitOrderIt->second->InstrumentID
				&& CW_FTDC_D_Buy == WaitOrderIt->second->Direction)
			{
				if (bNeedCancel
					&& m_cwMainMarketData->BidPrice1 - WaitOrderIt->second->LimitPrice > m_dSellThreadHold - dInsEQ)
				{
					bNeedCancel &= false;
				}

				if (bNeedCancel
					|| m_cwSubMainMarketData->BidPrice1 - WaitOrderIt->second->LimitPrice > 2 * dSubMainTickSize - dInsEQ)
				{
					CancelOrder(WaitOrderIt->second);
					log.AddLog(cwStrategyLog::enCO, "%s, Ref:%s, P:%.2f, V:%d %s", WaitOrderIt->second->InstrumentID, WaitOrderIt->second->OrderRef,
						WaitOrderIt->second->LimitPrice, WaitOrderIt->second->VolumeTotal, WaitOrderIt->second->Direction == CW_FTDC_D_Buy ? "B" : "S");
				}
				iSubMainWaitLongOrder += WaitOrderIt->second->VolumeTotal;

			}
		}

		//�򿪲�
		if (bCanOpen
			|| (m_cwMainMarketData->BidPrice1 - m_cwSubMainMarketData->AskPrice1 > m_dSellThreadHold - dInsEQ))
		{
			bCanOpen |= true;
		}

		if (bCanOpen
			&& iSubMainWaitLongOrder == 0
			&& iSubMainPosition + iSubMainWaitLongOrder < m_iPositionLimit)
		{
			double dbOrderPrice = m_cwSubMainMarketData->AskPrice1;

			int iVol = m_iPositionLimit - (iSubMainWaitLongOrder + iSubMainPosition);
			if (iVol > m_iOrderVolume)
			{
				iVol = m_iOrderVolume;
			}

			cwOrderPtr orderptr = EasyInputOrder(m_SubMainInstrumentID.c_str(), iVol, dbOrderPrice, m_cwSubMainOpenCloseMode);
			if (orderptr.get() != NULL)
			{
				log.AddLog(cwStrategyLog::enIO, "%s, Ref:%s, P:%.2f, V:%d %s", orderptr->InstrumentID, orderptr->OrderRef,
					orderptr->LimitPrice, orderptr->VolumeTotal, orderptr->Direction == CW_FTDC_D_Buy ? "B" : "S");
			}
		}
	}
#ifdef _MSC_VER
#pragma endregion
#endif

#ifdef _MSC_VER
#pragma region BullSpread
#endif
	///ţ������������Զ�ں�Լ��
	if (iSubMainPosition > 0)
	{
		///��ǰ���ֲ�Ϊ��֣���ƽ��, ��ƽ�ֲ�����ƽ���������ڿ�������
		bool bNeedCancel = false;
		bool bCanOpen = false;
		//�ȼ��ҵ�
		int iSubMainWaitShortOrder = 0;
		for (auto WaitOrderIt = WaitOrderList.begin();
			WaitOrderIt != WaitOrderList.end(); WaitOrderIt++)
		{
			bNeedCancel = true;
			if (m_SubMainInstrumentID == (std::string)WaitOrderIt->second->InstrumentID
				&& CW_FTDC_D_Sell == WaitOrderIt->second->Direction)
			{
				if (bNeedCancel
					&& m_cwMainMarketData->AskPrice1 - WaitOrderIt->second->LimitPrice < m_dBuyThreadHold + dInsEQ)
				{
					bNeedCancel &= false;
				}

				if (bNeedCancel
					|| WaitOrderIt->second->LimitPrice - m_cwSubMainMarketData->AskPrice1 > 2 * dSubMainTickSize - dInsEQ)
				{
					CancelOrder(WaitOrderIt->second);
					log.AddLog(cwStrategyLog::enCO, "%s, Ref:%s, P:%.2f, V:%d %s", WaitOrderIt->second->InstrumentID, WaitOrderIt->second->OrderRef,
						WaitOrderIt->second->LimitPrice, WaitOrderIt->second->VolumeTotal, WaitOrderIt->second->Direction == CW_FTDC_D_Buy ? "B" : "S");
				}
				iSubMainWaitShortOrder -= WaitOrderIt->second->VolumeTotal;
			}
		}

		//��ƽ��
		if (bCanOpen
			|| (m_cwMainMarketData->AskPrice1 - m_cwSubMainMarketData->BidPrice1 < m_dBuyThreadHold + dInsEQ))
		{
			bCanOpen |= true;
		}

		if (bCanOpen
			&& iSubMainWaitShortOrder == 0)
		{
			double dbOrderPrice = m_cwSubMainMarketData->BidPrice1;

			int iVol = iSubMainPosition + iSubMainWaitShortOrder;
			if (iVol > m_iOrderVolume)
			{
				iVol = m_iOrderVolume;
			}
			cwOrderPtr orderptr = EasyInputOrder(m_SubMainInstrumentID.c_str(), iVol * (-1), dbOrderPrice, m_cwSubMainOpenCloseMode);
			if (orderptr.get() != NULL)
			{
				log.AddLog(cwStrategyLog::enIO, "%s, Ref:%s, P:%.2f, V:%d %s", orderptr->InstrumentID, orderptr->OrderRef,
					orderptr->LimitPrice, orderptr->VolumeTotal, orderptr->Direction == CW_FTDC_D_Buy ? "B" : "S");
			}
		}
	}
	else
	{
		///�ÿ��ֲ���
		bool bNeedCancel = true;
		bool bCanOpen = false;
		//�ȼ��ҵ�
		int iSubMainWaitShortOrder = 0;
		for (auto WaitOrderIt = WaitOrderList.begin();
			WaitOrderIt != WaitOrderList.end(); WaitOrderIt++)
		{
			bNeedCancel = true;
			if (m_SubMainInstrumentID == (std::string)WaitOrderIt->second->InstrumentID
				&& CW_FTDC_D_Sell == WaitOrderIt->second->Direction)
			{
				if (bNeedCancel
					&& m_cwMainMarketData->AskPrice1 - WaitOrderIt->second->LimitPrice < m_dBuyThreadHold + dInsEQ)
				{
					bNeedCancel &= false;
				}

				if (bNeedCancel
					|| WaitOrderIt->second->LimitPrice - m_cwSubMainMarketData->AskPrice1 > 2 * dSubMainTickSize - dInsEQ)
				{
					CancelOrder(WaitOrderIt->second);
					log.AddLog(cwStrategyLog::enCO, "%s, Ref:%s, P:%.2f, V:%d %s", WaitOrderIt->second->InstrumentID, WaitOrderIt->second->OrderRef,
						WaitOrderIt->second->LimitPrice, WaitOrderIt->second->VolumeTotal, WaitOrderIt->second->Direction == CW_FTDC_D_Buy ? "B" : "S");
				}
				iSubMainWaitShortOrder -= WaitOrderIt->second->VolumeTotal;

			}
		}

		//������
		if (bCanOpen
			|| (m_cwMainMarketData->AskPrice1 - m_cwSubMainMarketData->BidPrice1 < m_dBuyThreadHold + dInsEQ))
		{
			bCanOpen |= true;
		}

		if (bCanOpen
			&& iSubMainWaitShortOrder == 0
			&& iSubMainPosition + iSubMainWaitShortOrder > m_iPositionLimit * -1)
		{
			double dbOrderPrice = m_cwSubMainMarketData->BidPrice1;

			int iVol = iSubMainPosition + iSubMainWaitShortOrder + m_iPositionLimit;
			if (iVol > m_iOrderVolume)
			{
				iVol = m_iOrderVolume;
			}

			cwOrderPtr orderptr = EasyInputOrder(m_SubMainInstrumentID.c_str(), iVol * (-1), dbOrderPrice, m_cwSubMainOpenCloseMode);
			if (orderptr.get() != NULL)
			{
				log.AddLog(cwStrategyLog::enIO, "%s, Ref:%s, P:%.2f, V:%d %s", orderptr->InstrumentID, orderptr->OrderRef,
					orderptr->LimitPrice, orderptr->VolumeTotal, orderptr->Direction == CW_FTDC_D_Buy ? "B" : "S");
			}
		}
	}

#ifdef _MSC_VER
#pragma endregion
#endif

}