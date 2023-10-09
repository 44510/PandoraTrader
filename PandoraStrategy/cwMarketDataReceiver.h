//////////////////////////////////////////////////////////////////////////////////
//*******************************************************************************
//---
//---	Create by Wu Chang Sheng on June.26th 2020
//---
//--	Copyright (c) by Wu Chang Sheng. All rights reserved.
//--    Consult your license regarding permissions and restrictions.
//--
//*******************************************************************************
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <fstream>
#include "cwBasicKindleStrategy.h"
#include "cwStrategyLog.h"
#include <iostream>
#include "tinyxml.h"

class cwMarketDataReceiver :
	public cwBasicKindleStrategy
{
public:
	cwMarketDataReceiver();
	~cwMarketDataReceiver();

	std::string  GetStrategyName();

	//MarketData SPI
	///�������
	virtual void PriceUpdate(cwMarketDataPtr pPriceData);
	//������һ����K�ߵ�ʱ�򣬻���øûص�
	virtual void			OnBar(cwMarketDataPtr pPriceData, int iTimeScale, cwBasicKindleStrategy::cwKindleSeriesPtr pKindleSeries);

	//Trade SPI
	///�ɽ��ر�
	virtual void OnRtnTrade(cwTradePtr pTrade) {};
	///�����ر�
	virtual void OnRtnOrder(cwOrderPtr pOrder, cwOrderPtr pOriginOrder = cwOrderPtr()) {};
	///�����ɹ�
	virtual void OnOrderCanceled(cwOrderPtr pOrder) {};

	virtual void OnReady();

	void InitialStrategy(const char * pConfigFilePath);


	std::string	m_strCurrentUpdateTime;

	///strategy parameter
	//�������д���
	std::string m_strStrategyName;		
	//�����Ƿ�����
	bool		m_bStrategyRun;					

	bool												m_bSaveInstrument = true;
private:
	cwStrategyLog										m_StrategyLog;

	std::unordered_map<std::string, uint64_t>			m_TotalVolume;
	std::unordered_map<std::string, double>				m_TotalTurnOver;

	std::unordered_map<std::string, bool>				m_bHasFirstQuotes;

	std::map<std::string, std::string>					m_HisMdFileIndex;

	std::string											m_strCurrentMdFilePath;
	std::string											m_strdateIndexId;
};

