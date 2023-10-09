//////////////////////////////////////////////////////////////////////////////////
//*******************************************************************************
//---
//---	Create by Wu Chang Sheng on May. 10th 2022
//---
//--	Copyright (c) by Wu Chang Sheng. All rights reserved.
//--    Consult your license regarding permissions and restrictions.
//--
//*******************************************************************************
//////////////////////////////////////////////////////////////////////////////////

//Note:
//�����ڷ��������ò���Դ�룬��������Ϊä�г��ۣ�����Լ3��
//������Ҫ�߼�Ϊ׷��ɱ�����ֽ�����Ҫ��������Ϊһ��ʾ�����ԣ������ο���
//�����ڽ��ף�������˽�����߼������ú���Ʒ�ּ����������Ը�ӯ����

#pragma once
#include "cwBasicKindleStrategy.h"

class cwChasingRisingStrategy :
    public cwBasicKindleStrategy
{
public:
	//��ȡ���԰汾��
	virtual std::string  GetStrategyVersion();
	//��ʾ��������
	virtual std::string  GetStrategyName();

	//MarketData SPI
	///�������
	virtual void			PriceUpdate(cwMarketDataPtr pPriceData);
	//������һ����K�ߵ�ʱ�򣬻���øûص�
	virtual void			OnBar(cwMarketDataPtr pPriceData, int iTimeScale, cwBasicKindleStrategy::cwKindleSeriesPtr pKindleSeries) {};


	//Trade SPI
	///�ɽ��ر�
	virtual void OnRtnTrade(cwTradePtr pTrade);
	///�����ر�
	virtual void OnRtnOrder(cwOrderPtr pOrder, cwOrderPtr pOriginOrder = cwOrderPtr()) {};
	///�����ɹ�
	virtual void OnOrderCanceled(cwOrderPtr pOrder) {};

	virtual void OnStrategyTimer(int iTimerId, const char* szInstrumentID) {};
	virtual void OnReady();

	virtual  void InitialStrategy(const char* pConfigFilePath);
	bool		IsNearDeliverDateWarning(const char* szInstrumentID);
	int			GetTradingDayRemainWarning(const char* szInstrumentID);



	///strategy parameter
	//�������д���
	std::string m_strStrategyName;
	//�����Ƿ�����, can be modified by config file
	bool		m_bStrategyRun;
	//��ʾ�ֲ֣�can be modified by config file
	bool		m_bShowPosition;

	std::string	m_strCurrentUpdateTime;
	std::string	m_strExeFolderPath;

	struct StrategyParameter
	{
		//general
		bool        Manual;							//�Ƿ��ֶ���Ԥ
		int			Portfolio;						//�ʲ����id

		int			TotalPositionLimit;				//�ֲܳ�����
		int			OrderVolume;					//������

		double		dStep;							//����

		//Instrument
		std::string Instrument;				//��������Լ
		cwOpenCloseMode OpenCloseMode;		//��ƽģʽ
		int			OpenCancelLimit;			//���ֳ�����������
		int			CloseCancelLimit;		//ƽ�ֳ�����������

		StrategyParameter()
			: Manual(false)
			, Portfolio(0)

			, TotalPositionLimit(0)
			, OrderVolume(1)

			, dStep(0.005)

			, OpenCloseMode(cwOpenCloseMode::CloseTodayThenYd)
			, OpenCancelLimit(350)
			, CloseCancelLimit(380)
		{

		}
	};
	typedef std::shared_ptr<StrategyParameter>			StrategyParaPtr;

	//Strategy Info Update By Strategy
	struct RunningParameter
	{
		cwMarketDataPtr LastMarketData;

		int64_t			baseTime;
		int64_t			oldTime;
		double			basePrice;
		double			dHighPx;
		double			dLowPx;
		std::string		strBaseTime;
		bool			bFirst;

		int				iTradeCnt;

		RunningParameter()
			: baseTime(0)
			, oldTime(0)
			, basePrice(0.0)
			, dHighPx(0.0)
			, dLowPx(0.0)
			, bFirst(true)
			, iTradeCnt(0)
		{
		}

	};
	typedef std::shared_ptr<RunningParameter>			cwRunningParaPtr;


	StrategyParameter									m_cwStrategyParameter;
	cwRunningParaPtr									m_cwRunningParaPtr;

	std::map<std::string, StrategyParaPtr>				m_StrategyParameterMap;		//key Instrument value:StrategyParameter
	std::map<std::string, cwRunningParaPtr>				m_cwRunningParameterMap;	//key Instrument value Running Parameter


	//Get Strategy Config
	bool ReadXmlConfigFile(const char* pConfigFilePath, bool bNeedDisPlay = true);
	bool				m_bFirstGetConfig;
	time_t				m_tLastestGetConfigTime;


	bool GetParameter(const char* szInstrumentID);

	void ChasingRising();

	int64_t TimeToint64(cwMarketDataPtr pData);

private:

	cwStrategyLog			m_StrategyLog;
	cwBasicCout				m_cwShow;

	int						m_iDoChasingRisingCount;


};

