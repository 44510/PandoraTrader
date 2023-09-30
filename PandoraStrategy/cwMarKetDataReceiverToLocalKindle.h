#pragma once
#include "cwMarketDataReceiver.h"
#include "cwBasicCout.h"
#include <string>


class cwMarKetDataReceiverToLocalKindle :
	public cwMarketDataReceiver
{
public:
	cwMarKetDataReceiverToLocalKindle();
	~cwMarKetDataReceiverToLocalKindle();

	std::string  GetStrategyName();

	//MarketData SPI
	///�������
	virtual void PriceUpdate(cwMarketDataPtr pPriceData);

	//������һ����K�ߵ�ʱ�򣬻���øûص�
	virtual void			OnBar(cwMarketDataPtr pPriceData, int iTimeScale, cwBasicKindleStrategy::cwKindleSeriesPtr pKindleSeries);
	virtual void			OnReady();

	void InitialStrategy(const char * pConfigFilePath);

	bool					InitialHisKindleFromHisKindleFolder(const char* szHisFolder);

	cwBasicCout				m_cwShow;

	struct WriterControlFiled
	{
		int TotalCount;				//����
		int Finished;				//����ɣ���д�룩��

		int WriteCount;				//��д����
		int NothingToWriteCount;	//��xx�Σ������ݿ�д����


		WriterControlFiled()
		{
			TotalCount = 0;
			Finished = -1;

			WriteCount = -1;
			NothingToWriteCount = 0;
		}
	};


	std::string									m_strWorkingPath;
	std::string									m_strExeFolderPath;

	std::unordered_map<std::string, std::string> m_KindleFileMap;


	cwMUTEX													m_DequeMutex;
	std::unordered_map<std::string, WriterControlFiled>		m_KindleFinishedIndex;

	std::thread									m_WorkingThread;			//�����������߳�
	volatile std::atomic<bool>					m_bWorkingThreadRun;		//�������߳�����״̬

	void										WorkingThread();



};

