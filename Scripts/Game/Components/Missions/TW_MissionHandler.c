class TW_MissionHandlerComponentClass : ScriptComponentClass {};

class TW_MissionHandlerComponent : ScriptComponent
{
	[Attribute("", UIWidgets.Auto, category: "Missions", desc: "Available missions that may spawn in game", params: "conf class=TW_Mission")]
	private ref array<ResourceName> m_MissionPool;
	
	[Attribute("2", UIWidgets.Slider, category: "Missions", desc: "Maximum number of missions that can be active at a time", params: "0 5 1")]
	private int m_MaxMissionsAllowed;
	
	[Attribute("10", UIWidgets.Slider, category: "Time", desc: "Interval in which a new mission will spawn, in seconds", params: "10 600 5")]
	private int m_Interval;
	
	//! Missions that are active in the world. Key: MissionID
	private ref map<int, ref TW_Mission>> m_ActiveMissions = new ref map<int, ref TW_Mission>>();
	private int lastMissionId = 0;
	
	override void OnPostInit(IEntity owner)
	{
		if(!TW_Global.IsInRuntime() || !TW_Global.IsServer(owner))
			return;
		// Infinitely check missions
		GetGame().GetCallqueue().CallLater(CheckMissions, SCR_TW_Util.FromSecondsToMilliseconds(30), true);
		GetGame().GetCallqueue().CallLater(NewRandomMission, SCR_TW_Util.FromSecondsToMilliseconds(m_Interval), true);
	}
	
	private void CheckMissions()
	{
		foreach(int missionId, ref TW_Mission mission : m_ActiveMissions)
		{
			if(!mission.IsSuccess())
				continue;
			
			m_ActiveMissions.Remove(missionId);
		}
	}
	
	//! Can another mission be spawned?
	bool CanSpawnMission()
	{
		return m_ActiveMissions.Count() < m_MaxMissionsAllowed;
	}
	
	void NewRandomMission()
	{
		if(!CanSpawnMission())
			return;
		
		ResourceName missionConf = m_MissionPool.GetRandomElement();
		Resource missionResource = BaseContainerTools.LoadContainer(missionConf);
		
		if(!missionResource)
		{
			Print(string.Format("TrainWreck: Failed to load mission conf file %1", missionConf), LogLevel.ERROR);
			return;
		}
		
		ref TW_Mission mission = TW_Mission.Cast(BaseContainerTools.CreateInstanceFromContainer(missionResource.GetResource().ToBaseContainer()));
		
		if(!mission)
		{
			Print(string.Format("TrainWreck: Failed to load load mission conf file as resource %1", missionConf), LogLevel.ERROR);
			return;
		}
		
		lastMissionId++;
		mission.SetMissionId(lastMissionId);
		m_ActiveMissions.Insert(lastMissionId, mission);
		mission.InitializeMission();
	}
};