class SCR_TW_ExtractionSpawnHandlerClass : SCR_BaseGameModeComponentClass {};
class SCR_TW_ExtractionSpawnHandler : SCR_BaseGameModeComponent
{
	[Attribute("", UIWidgets.CheckBox, category: "Spawn Details")]
	protected bool m_DisableSpawns;
	
	[Attribute("10", UIWidgets.Slider, params: "5 75 1", category: "Spawn Details")]
	protected int m_MaxAgents;
	
	[Attribute("2", UIWidgets.Slider, params: "1 30 1", category: "Spawn Details", desc: "Time in minutes for checking when to spawn AI")]
	protected int m_SpawnTimerInMinutes;
	
	[Attribute("300", UIWidgets.Slider, params: "0 1000 10", category: "Spawn Details", desc: "Distance in meters in which AI cannot be spawned")]
	protected int m_MinimumAISpawnDistance;
	
	[Attribute("500", UIWidgets.Slider, params: "0 3000 10", category: "Garbage Collection", desc: "If this distance is exceeded, units will be queued for removal")]
	protected int m_MaximumAIDistance;
	
	[Attribute("15", UIWidgets.Slider, params: "0 500 1", category: "Garbage Collection", desc: "Time in seconds. If AI exceed max distance for this time they'll be cleaned up")]
	protected float m_ExceedDistanceGarbageTimerInSeconds;
	
	[Attribute("120", UIWidgets.Slider, params: "0 500 1", category: "Garbage Collection", desc: "Time in seconds. Interval GC is checked (nothing happens)")]
	protected float m_GarbageCollectionTimer;
	
	protected ref array<SCR_AIGroup> m_CurrentGroups = {};
	protected ref array<SCR_TW_AISpawnPoint> m_AISpawnPoints = {};
	protected ref array<IEntity> players;
	
	static SCR_TW_ExtractionSpawnHandler sInstance;
	
	static SCR_TW_ExtractionSpawnHandler GetInstance()
	{
		if(sInstance) return sInstance;
		
		sInstance = SCR_TW_ExtractionSpawnHandler.Cast(GetGame().GetGameMode().FindComponent(SCR_TW_ExtractionSpawnHandler));
		
		return sInstance;
	}
	
	override void OnGameModeStart()
	{		
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		
		if(!rpl.IsMaster())
			return;		
		
		if(m_DisableSpawns)
			return;
		
		GetGame().GetCallqueue().CallLater(FirstPass, 15 * 1000, false);
		
		// These methods periodically update our stuff. Thus need to repeat indefinitely
		GetGame().GetCallqueue().CallLater(ReinitializePlayers, SCR_TW_Util.FromSecondsToMilliseconds(m_GarbageCollectionTimer), true);
		GetGame().GetCallqueue().CallLater(GarbageCollection, SCR_TW_Util.FromSecondsToMilliseconds(m_GarbageCollectionTimer), true);
		GetGame().GetCallqueue().CallLater(SpawnLoop, SCR_TW_Util.FromMinutesToMilliseconds(m_SpawnTimerInMinutes), true);
	}
	
	void FirstPass()
	{
		ReinitializePlayers();
		SpawnLoop();
	}
	
	void ProcessForGC(AIAgent agent)
	{
		if(!agent) return;
		
		if(!SCR_TW_Util.IsOutsideOfPlayers(agent.GetOrigin(), players, m_ExceedDistanceGarbageTimerInSeconds))
			return;
		
		SCR_EntityHelper.DeleteEntityAndChildren(agent);
	}
	
	void SpawnLoop()
	{
		ref array<AIAgent> agents = {};
		int currentAgents = GetAgentCount(agents);
		
		if(currentAgents >= m_MaxAgents)
		{
			Print(string.Format("TrainWreck: Spawn system is capped at %1/%2", currentAgents, m_MaxAgents), LogLevel.WARNING);
			return;
		}
		
		ref array<SCR_TW_AISpawnPoint> nearbyPoints = {};
		int nearbyCount = GetPointsInPlayerVicinity(nearbyPoints);
		
		if(nearbyCount == 0)
		{
			Print("TrainWreck: Failed to find nearby SCR_TW_AISpawnPoints", LogLevel.WARNING);
			return;
		}
		
		int spawnCount = Math.RandomIntInclusive(0, Math.Min(10, nearbyCount));
		for(int i = 0; spawnCount; i++)
		{
			if(currentAgents >= m_MaxAgents) break;
			if(nearbyCount <= 0) break;
			
			int index = nearbyPoints.GetRandomIndex();
			SCR_TW_AISpawnPoint currentPoint = nearbyPoints.Get(index);
			nearbyPoints.Remove(index);
			nearbyCount--;
			
			if(!currentPoint.CanSpawn())
				continue;
			
			SCR_AIGroup group = currentPoint.Spawn();
			
			if(!group)
				continue;
			
			currentAgents += group.GetAgentsCount();
		}
	}
	
	int GetPointsInPlayerVicinity(notnull array<SCR_TW_AISpawnPoint> points)
	{
		int count = 0;
		foreach(SCR_TW_AISpawnPoint point : m_AISpawnPoints)
		{
			if(!point) continue;
			if(!SCR_TW_Util.IsWithinRange(point.GetOrigin(), players, m_MinimumAISpawnDistance, m_MaximumAIDistance))
				continue;
			points.Insert(point);
			count++;
		}
		
		return count;
	}
	
	void GarbageCollection()
	{
		ref array<AIAgent> agents = {};
		int currentAgents = GetAgentCount(agents);
		int queuedForGC = 0;
		
		if(currentAgents <= 0)
			return;
		
		Print("TrainWreck: GarbageCollection Pass");
		
		foreach(AIAgent agent : agents)
		{
			if(!agent)
				continue;
			
			if(SCR_TW_Util.IsOutsideOfPlayers(agent.GetOrigin(), players, m_MaximumAIDistance))
			{				
				GetGame().GetCallqueue().CallLater(ProcessForGC, SCR_TW_Util.FromSecondsToMilliseconds(m_ExceedDistanceGarbageTimerInSeconds), false, agent);
				queuedForGC++;
			}
		}
		
		Print(string.Format("TrainWreck: AI Agents Detected: %1. Queued for GC: %2", currentAgents, queuedForGC), LogLevel.WARNING);
	}
	
	void ReinitializePlayers()
	{
		ref array<int> playerIds = {};
		ref array<IEntity> players = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		
		foreach(int playerId : playerIds)
		{
			auto player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			
			if(!player)
				continue;
			
			players.Insert(player);
		}
		
		Print(string.Format("TrainWreck: Players: %1", players.Count()));
		this.players = players;
	}
	
	void RegisterAISpawnPoint(SCR_TW_AISpawnPoint point)
	{
		if(!point)
		{
			Print("TrainWreck: invalid spawn point. Cannot be null", LogLevel.ERROR);
			return;
		}
		
		m_AISpawnPoints.Insert(point);
	}
	
	void UnregisterAISpawnPoint(SCR_TW_AISpawnPoint point)
	{
		if(!point) return;
		
		m_AISpawnPoints.RemoveItem(point);
	}
	
	int GetAgentCount(out notnull array<AIAgent> agents)
	{
		ref array<AIAgent> worldAgents = {};
		GetGame().GetAIWorld().GetAIAgents(worldAgents);
		
		// We want to ensure we're only grabbing AI we care about -- specifically AI
		foreach(auto agent : worldAgents)
		{
			SCR_ChimeraAIAgent ai = SCR_ChimeraAIAgent.Cast(agent);
			if(!ai)
				continue;
			
			agents.Insert(agent);
		}
		
		return agents.Count();
	}
	
};