//////////////////////////////////////////////////////////////////////////////////
//*******************************************************************************
//---
//---	Create by Wu Chang Sheng on May. 20th 2020
//---
//--	Copyright (c) by Wu Chang Sheng. All rights reserved.
//--    Consult your license regarding permissions and restrictions.
//--
//--	������Agent֮�󣬵���SetExpectPosition����ʵ�ֶԸú�Լ��λ����
//*******************************************************************************
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "cwBasicAgent.h"
#include "cwBasicStrategy.h"

class cwPandoraPositionAgent :
	public cwBasicAgent
{
public:
	cwPandoraPositionAgent();
	~cwPandoraPositionAgent();

	virtual void			PriceUpdate(cwMarketDataPtr pPriceData);
	virtual void			OnRtnTrade(cwTradePtr pTrade);
	virtual void			OnRtnOrder(cwOrderPtr pOrder, cwOrderPtr pOriginOrder = cwOrderPtr());
	virtual void			OnOrderCanceled(cwOrderPtr pOrder);
	virtual void			OnRspOrderInsert(cwOrderPtr pOrder, cwRspInfoPtr pRspInfo);
	virtual void			OnRspOrderCancel(cwOrderPtr pOrder, cwRspInfoPtr pRspInfo);


	void					SetExpectPosition(int iExpPos = 0);

	int						m_iExpectPosition;
	std::string				m_strInstrumentID;

	cwBasicStrategy::cwOpenCloseMode OpenCloseMode;			//��ƽģʽ
	int			InsLargeOrderVolume;		//��������������Ϊ��
	int			InsLittleOrderVolume;		//С������С������ΪС��
	int			InsAskBidGap;				//�̿ڼ۲�

protected:
	void					DealExpectedPosition(std::string InstrumentID, int iExpectedMaintain = 0, const char * szCallMsg = NULL);
	std::string				m_strCurrentUpdateTime;

};

