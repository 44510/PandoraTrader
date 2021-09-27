// cwPegasusSimulator.cpp : Defines the entry point for the  Pandora Simulator console application.
//
//For more information, please visit https://github.com/pegasusTrader/PandoraTrader
//
//Please use the platform with legal and regulatory permission.
//This software is released into the public domain.You are free to use it in any way you like, except that you may not sell this source code.
//This software is provided "as is" with no expressed or implied warranty.I accept no liability for any damage or loss of business that this software may cause.
//

//#include "stdafx.h"

#include "cwPegasusSimulator.h"
#include "cwSimMdSpi.h"
#include "cwSimTradeSpi.h"
#include "cwEmptyStrategy.h"
#include "tinyxml.h"
#include "cwBasicCout.h"

#ifdef _MSC_VER
#pragma comment(lib, "cwPandoraDLL.lib")
#pragma comment(lib, "PandoraStrategy.lib")
#pragma comment(lib, "tinyxml.lib")
#endif // WIN32

cwBasicCout				m_cwShow;

cwFtdcBrokerIDType		m_szMdBrokerID;
cwFtdcUserIDType		m_szMdUserID;
cwFtdcPasswordType		m_szMdPassWord;

cwPegasusSimulator		m_PegasusSimulator;
cwSimMdSpi				m_mdCollector;
cwSimTradeSpi			m_TradeChannel;
cwEmptyStrategy			m_Strategy;

std::vector<std::string> m_SubscribeInstrument;

std::string				m_strStrategyConfigFile;

unsigned int PriceServerThread()
{

	m_TradeChannel.Connect(&m_PegasusSimulator);


	//m_SubscribeInstrument.push_back("zn1904");
	//m_SubscribeInstrument.push_back("SR909");

	m_mdCollector.SetUserLoginField(m_szMdBrokerID, m_szMdUserID, m_szMdPassWord);
	m_mdCollector.SubscribeMarketData(m_SubscribeInstrument);

	m_mdCollector.Connect(&m_PegasusSimulator);

	m_PegasusSimulator.SimulationStart();

	m_mdCollector.WaitForFinish();

	return 0;
}

int main()
{

	char exeFullPath[MAX_PATH];
	memset(exeFullPath, 0, MAX_PATH);
	std::string strFullPath;
#ifdef WIN32
	WCHAR TexeFullPath[MAX_PATH] = { 0 };

	GetModuleFileName(NULL, TexeFullPath, MAX_PATH);
	int iLength;
	//��ȡ�ֽڳ���   
	iLength = WideCharToMultiByte(CP_ACP, 0, TexeFullPath, -1, NULL, 0, NULL, NULL);
	//��tcharֵ����_char    
	WideCharToMultiByte(CP_ACP, 0, TexeFullPath, -1, exeFullPath, iLength, NULL, NULL);
#else
	size_t cnt = readlink("/proc/self/exe", exeFullPath, MAX_PATH);
	if (cnt < 0 || cnt >= MAX_PATH)
	{
		printf("***Error***\n");
		exit(-1);
	}
#endif // WIN32

	strFullPath = exeFullPath;
	strFullPath = strFullPath.substr(0, strFullPath.find_last_of("/\\"));

#ifdef WIN32
	strFullPath.append("\\PandoraSimulatorConfig.xml");
#else
	strFullPath.append("/PandoraSimulatorConfig.xml");
#endif // WIN32

	m_Strategy.InitialStrategy(NULL);

	m_PegasusSimulator.InitialSimulator(strFullPath.c_str());
	m_PegasusSimulator.SetMdSpi((void*)(&m_mdCollector));
	m_PegasusSimulator.SetTradeSpi((void*)&m_TradeChannel);

	m_TradeChannel.RegisterBasicStrategy(dynamic_cast<cwBasicStrategy*>(&m_Strategy));

	m_mdCollector.RegisterTradeSPI(dynamic_cast<cwBasicTradeSpi*>(&m_TradeChannel));
	m_mdCollector.RegisterStrategy(dynamic_cast<cwBasicStrategy*>(&m_Strategy));

	std::thread m_PriceServerThread = std::thread(PriceServerThread);
	//std::thread m_TradeServerThread = std::thread(TradeServerThread);

	while (true)
	{
		cwSleep(3000);
		m_cwShow.AddLog("%s %s Ȩ��:%.3f ƽ��ӯ��:%.3f �ֲ�ӯ��:%.3f",
			m_PegasusSimulator.m_CurrentTradingDay,
			m_PegasusSimulator.m_CurrentSimulationTime,
			m_PegasusSimulator.m_cwSettlement.m_dBalance,
			m_PegasusSimulator.m_cwSettlement.m_dCloseProfit,
			m_PegasusSimulator.m_cwSettlement.m_dPositionProfit);
	}
    return 0;
}

