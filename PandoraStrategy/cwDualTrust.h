#pragma once
#include "cwBasicCTAStrategy.h"

class cwDualTrust :
    public cwBasicCTAStrategy
{
public:
	cwDualTrust(const char * szStrategyName);
	//������һ����K�ߵ�ʱ�򣬻���øûص�
	virtual void			OnBar(bool bFinished, int iTimeScale, cwBasicKindleStrategy::cwKindleSeriesPtr pKindleSeries);


};

