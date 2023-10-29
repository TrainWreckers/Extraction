[EntityEditorProps(category: "GameScripted/TrainWreck/Encounters", description: "Handles a specific encounter")]
class SCR_TW_EncounterHandlerClass : GenericEntityClass
{

};

class SCR_TW_EncounterHandler : GenericEntity
{
	protected map<FactionKey, ref array<SCR_CampaignMilitaryBaseComponent>> baseDict;
	protected ref array<IEntity> players;
	
	protected FactionKey playerFaction;
	protected FactionKey opposingFaction;
	
	[Attribute("30", UIWidgets.Slider, params: "1 60 1", category: "Configs")]
	protected int intervalCheck;
	
	[Attribute("", UIWidgets.Object, params: "confg class=TW_EncounterConfig", category: "Configs")]
	protected ref TW_EncounterConfig m_Config;
	
	protected ref TW_FactionReinforcementsConfig m_GroupPrefabsConfig;
	
	[Attribute("", UIWidgets.Object, params: "confg class=TW_GroupWaypoints", category: "Configs")]
	protected ref TW_GroupWaypoints m_WaypointConfig;
	
	protected bool m_HasErrored;
	protected ref array<SCR_AIGroup> m_SpawnedGroups = new ref array<SCR_AIGroup>;
	protected SCR_BaseTask m_Task;
	protected SCR_TW_EncounterPhase currentPhase = SCR_TW_EncounterPhase.Initialize;
	
	bool HasErrored() { return m_HasErrored; }
	
	TW_EncounterConfig GetEncounterConfig() { return m_Config; }
	TW_GroupWaypoints GetWaypointsConfig() { return m_WaypointConfig; }
	
	int AliveGroups()
	{
		foreach(SCR_AIGroup group : m_SpawnedGroups)
		{
			if(!group)
			{
				m_SpawnedGroups.RemoveItem(group);
				continue;
			}
			
			if(group.GetAgentsCount() <= 0)
			{
				m_SpawnedGroups.RemoveItem(group);
				continue;
			}
		}
		
		return m_SpawnedGroups.Count();
	}
	
	protected bool IsMaster()
	{
		RplComponent rpl = RplComponent.Cast(FindComponent(RplComponent));
		if(rpl && !rpl.IsMaster())
			return false;
		
		return true;
	}
	
	protected void SelectGroupConfig()
	{
		m_GroupPrefabsConfig = SCR_TW_AmbientEncounters.GetInstance().GetIndforConfig();
	}
	
	void Setup(notnull map<FactionKey, ref array<SCR_CampaignMilitaryBaseComponent>> baseDict, FactionKey playerFaction, FactionKey opposingFaction, notnull array<IEntity> players)
	{
		if(!IsMaster())
		{
			m_HasErrored = true;
			return;
		}
		
		this.baseDict = baseDict;
		this.players = players;	
		this.playerFaction = playerFaction;
		this.opposingFaction = opposingFaction;
		
		if(!m_Config)
		{
			m_HasErrored = true;
			Print("A configuration object is required for this encounter to work properly. Exiting...", LogLevel.ERROR);
		}
		
		SelectGroupConfig();
		GetGame().GetCallqueue().CallLater(Handle, SCR_TW_Util.FromSecondsToMilliseconds(intervalCheck), true);
	}
	
	void Handle()
	{
		int aliveCount = AliveGroups();
		
		Print(string.Format("Interval Check %1: Alive(%2) > %3", m_Config.GetEncounterType(), aliveCount, currentPhase), LogLevel.DEBUG);
		
		SCR_TW_AmbientEncounters.GetInstance().RegisterForGarbageCollection(m_SpawnedGroups);		
	}
	
	void Dispose()
	{
		if(m_Task)
		{
			GetTaskManager().DeleteTask(m_Task);
		}
		
		delete m_Task;
		delete m_Config;
	}
	
	bool IsComplete()
	{
		if(currentPhase != SCR_TW_EncounterPhase.Complete)
			return false;
		return true;
	}
};