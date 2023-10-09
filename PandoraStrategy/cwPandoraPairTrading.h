//////////////////////////////////////////////////////////////////////////////////
//*******************************************************************************
//---
//---	Create by Wu Chang Sheng on May. 20th 2020
//---
//--	Copyright (c) by Wu Chang Sheng. All rights reserved.
//--    Consult your license regarding permissions and restrictions.
//--
//*******************************************************************************
//////////////////////////////////////////////////////////////////////////////////

//PairTrading may use for arbitrage

#pragma once
#include "cwBasicKindleStrategy.h"
#include "cwStrategyLog.h"
#include "cwBasicCout.h"
#include "cwPandoraAgentManager.h"

class cwPandoraPairTrading :
	public cwBasicKindleStrategy
{
public:
	cwPandoraPairTrading();
	~cwPandoraPairTrading();


	///MarketData SPI
	//�������
	virtual void PriceUpdate(cwMarketDataPtr pPriceData);

	///Trade SPI
	//�ɽ��ر�
	virtual void OnRtnTrade(cwTradePtr pTrade) {};
	//�����ر�, pOrderΪ���±�����pOriginOrderΪ��һ�θ��±����ṹ�壬�п���ΪNULL
	virtual void OnRtnOrder(cwOrderPtr pOrder, cwOrderPtr pOriginOrder = cwOrderPtr()) {};
	//�����ɹ�
	virtual void OnOrderCanceled(cwOrderPtr pOrder) {};
	//����¼��������Ӧ
	virtual void OnRspOrderInsert(cwOrderPtr pOrder, cwFtdcRspInfoField * pRspInfo) {};
	//��������������Ӧ
	virtual void OnRspOrderCancel(cwOrderPtr pOrder, cwFtdcRspInfoField * pRspInfo) {};
	//�����Խ��׳�ʼ�����ʱ�����OnReady, �����ڴ˺��������Եĳ�ʼ������
	virtual void OnReady();
	
	//���Խ��״�������Լ
	void		 DoManualSpread();

	std::string					m_strCurrentUpdateTime;			//��������ʱ��

protected:
	std::string					m_MainInstrumentID;				//������Լ
	std::string					m_SubMainInstrumentID;			//��������Լ

	//�۲��Ϊ����-������
	double						m_dBuyThreadHold;				//�۲�����ֵ
	double						m_dSellThreadHold;				//�۲�����ֵ
	
	double						m_dVolumeCoefficient;			//�Գ����

	cwMarketDataPtr				m_cwMainMarketData;				//������Լ����
	cwMarketDataPtr				m_cwSubMainMarketData;			//��������Լ����

	cwOpenCloseMode				m_cwMainOpenCloseMode;			//������ƽģʽ
	cwOpenCloseMode				m_cwSubMainOpenCloseMode;		//��������ƽģʽ

	int							m_iPositionLimit;				//�ֲ�����
	int							m_iOrderVolume;					//��������

	cwStrategyLog				m_StrategyLog;					//������־
	cwBasicCout					m_cwShow;						//cout


	cwPandoraAgentManager					m_PandoraAgentManager;		//�����˹����ߣ���ͨ��������������

	cwPandoraAgentManager::cwAgentDataPtr	m_pPositionAgent;			//��λ��������ˣ�Ҫָ����Լ
};

