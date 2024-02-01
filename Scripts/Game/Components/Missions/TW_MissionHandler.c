class TW_MissionHandlerComponentClass : ScriptComponentClass {};

class TW_MissionHandlerComponent : ScriptComponent
{
	[Attribute("", UIWidgets.Auto, category: "Missions", desc: "Available missions that may spawn in game", params: "conf class=TW_Mission")]
	private ref array<ResourceName> m_MissionPool;
	
	[Attribute("", UIWidgets.Auto, category: "Extractions", desc: "Available extractions", params: "conf class=TW_ExtractionCall")]
	private ref array<ref TW_ExtractionCall> m_Extractions;
	
	[Attribute("2", UIWidgets.Slider, category: "Missions", desc: "Maximum number of missions that can be active at a time", params: "0 5 1")]
	private int m_MaxMissionsAllowed;
	
	[Attribute("10", UIWidgets.Slider, category: "Time", desc: "Interval in which a new mission will spawn, in seconds", params: "10 600 5")]
	private int m_Interval;
	
	static TW_MissionHandlerComponent GetInstance() { return s_Instance; }
	protected static TW_MissionHandlerComponent s_Instance;
	
	//! Missions that are active in the world. Key: MissionID
	private ref map<int, ref TW_Mission>> m_ActiveMissions = new ref map<int, ref TW_Mission>>();
	private int lastMissionId = 0;
	
	//! Retrieve list of misions
	int GetMissionPool(notnull array<ResourceName> resources)
	{
		int count = 0;
		
		foreach(ResourceName resource : m_MissionPool)
		{
			resources.Insert(resource);
			count++;
		}
		
		return count;
	}
	
	//! Retrieve list of extractions
	int GetExtractionPool(notnull array<ref TW_ExtractionCall> extractions)
	{
		int count =0;
		
		foreach(ref TW_ExtractionCall call : m_Extractions)
		{
			extractions.Insert(call);
			count++;
		}
		
		return count;
	}
	
	override void OnPostInit(IEntity owner)
	{
		s_Instance = this;
		if(!TW_Global.IsInRuntime() || !TW_Global.IsServer(owner))
			return;
		
		// Infinitely check missions
		GetGame().GetCallqueue().CallLater(CheckMissions, SCR_TW_Util.FromSecondsToMilliseconds(30), true);
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
	
	private TW_ExtractionCall GetExtractionByName(string name)
	{
		foreach(TW_ExtractionCall call : m_Extractions)
			if(call.GetExtractionName() == name)
				return call;
		return null;
	}
	
	void SpawnMission(ResourceName resourceName)
	{
		if(!CanSpawnMission())
			return;
		
		Resource missionResource = BaseContainerTools.LoadContainer(resourceName);
		
		if(!missionResource)
		{
			Print(string.Format("TrainWreck: Failed to load mission conf file %1", resourceName), LogLevel.ERROR);
			return;
		}
		
		ref TW_CallableItem callableItem = TW_CallableItem.Cast(BaseContainerTools.CreateInstanceFromContainer(missionResource.GetResource().ToBaseContainer()));
		
		ref TW_Mission mission = TW_Mission.Cast(callableItem);
		
		if(!mission)
		{
			Print(string.Format("TrainWreck: Failed to load load mission conf file as resource %1", resourceName), LogLevel.ERROR);
			return;
		}
		
		lastMissionId++;
		mission.SetMissionId(lastMissionId);
		m_ActiveMissions.Insert(lastMissionId, mission);
		mission.InitializeMission();
	}
};