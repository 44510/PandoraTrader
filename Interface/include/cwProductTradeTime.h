//////////////////////////////////////////////////////////////////////////////////
//*******************************************************************************
//---
//---	author: Wu Chang Sheng
//---
//--	Copyright (c) by Wu Chang Sheng. All rights reserved.
//--    Consult your license regarding permissions and restrictions.
//--
//*******************************************************************************
//////////////////////////////////////////////////////////////////////////////////

#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include "cwMarketTime.h"

//Ʒ�ֽ���ʱ��
class cwProductTradeTime
{

public:
	cwProductTradeTime();
	~cwProductTradeTime();

	enum cwTradeTimeSpace
	{
		NoTrading = 0										//�ǽ���ʱ��
		, NightPartOne										//ҹ��
		, AMPartOne											//�����һ�׶�
		, AMPartTwo											//����ڶ��׶�
		, PMPartOne											//�����һ�׶�
		, CallAuctionOrderingOpen							//���Ͼ��۱��������̣�
		, CallAuctionMatchOpen								//���Ͼ��۴�ϣ����̣�
		, CallAuctionOrderingClose							//���Ͼ��۱��������̣�
		, CallAuctionMatchClose								//���Ͼ��۴�ϣ����̣�
		, TradeTimeSpaceCnt
	};

	enum cwRangeOpenClose
	{
		cwLeftOpenRightOpen = 0,							//(a,b)
		cwLeftOpenRightClose,								//(a,b]
		cwLeftCloseRightOpen,								//[a,b)
		cwLeftCloseRightClose								//[a,b]
	};

	typedef struct tagProductTradeTime
	{
		cwTradeTimeSpace TradeTimeSpace;
		cwRangeOpenClose RangeOpenClose;

		cwMarketTimePtr BeginTime;
		cwMarketTimePtr EndTime;

	}ProductTradeTime;
	typedef std::shared_ptr<ProductTradeTime> TradeTimePtr;

	bool GetTradeTimeSpace(std::string ProductId, std::string updatetime,
		cwTradeTimeSpace& iTradeIndex, int& iOpen, int& iClose);
	int	 GetPreTimeSpaceInterval(std::string ProductId, cwTradeTimeSpace iTradeIndex);
	int	 GetTimeSpaceInterval(std::string productId, std::string starttime, std::string endTime);

private:
	std::unordered_map<std::string, std::vector<TradeTimePtr>> m_ProductTradeTimeMap;
	void InitialTradeTimeMap();
};

