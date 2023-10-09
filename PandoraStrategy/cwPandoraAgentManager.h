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
#include "cwAgentManager.h"
#include "cwPandoraPositionAgent.h"

class cwPandoraAgentManager :
	public cwAgentManager
{
public:
	cwPandoraAgentManager();
	~cwPandoraAgentManager();

#ifdef _MSC_VER
#pragma region CommenDefine
#endif // _MSC_VER
	typedef std::shared_ptr<cwPandoraPositionAgent>		cwPositionAgentPtr;

	enum cwPandoraAgentEnum : int
	{
		Enum_Agent_Postion = 0,					//cwPandoraPositionAgent		���㷨������ֲ�
		Enum_Agent_TakeOver,					//cwPandoraTakeOverAgent		���㷨���ӹֲ֣ܳ��Զ�ֹӯֹ��,δ���
		Enum_Agent_Prerequisite,				//cwPandoraPrerequisiteAgent	���㷨�������������������Ҫ�������򳷵���δ���
		Enum_Agent_ProfitLost,					//cwPandoraProfitLostAgent		���㷨�����������ɽ����Զ�����ֹӯ��;������ֹ����ֹ�𵥣�δ���
		Enum_Agent_TWAP,
		Enum_Agent_VWAP,
		Enum_Agent_Count
	};

	struct cwAgentData
	{
		int							AgentID;			//�����˱��
		cwPandoraAgentEnum			AgentType;			//����������
		cwPositionAgentPtr			pPositionAgent;		//�ֲֹ��������
	};
	typedef std::shared_ptr<cwAgentData> cwAgentDataPtr;
#ifdef _MSC_VER
#pragma endregion
#endif

	cwAgentDataPtr			RegisterAgent(std::string instrumentid, cwPandoraAgentEnum agentEnum);

public:
	//key InstrumentID, key :AgentID value:agentData
	std::unordered_map<std::string, std::unordered_map<int, cwAgentDataPtr>>		m_cwPandoraAgentDataMap;

};

