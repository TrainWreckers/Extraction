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
	
	[Attribute("120", UIWidgets.Slider, params: "0 500 1", category: "Vehicle Spawn", desc: "Minimum distance vehicles may spawn")]
	protected int m_MinimumVehicleSpawnDistance;
	
	[Attribute("500", UIWidgets.Slider, params: "0 1500 1", category: "Vehicle Spawn", desc: "Maximum distance vehicles may spawn")]
	protected int m_MaximumVehicleSpawnDistance;	
	
	[Attribute("800", UIWidgets.Slider, params: "0 5000 10", category: "Garbage Collection", desc: "Vehicles exceeding this distance shall get removed")]
	protected int m_VehicleGarbageCollectionDistance;
	
	[Attribute("0.6", UIWidgets.Slider, params: "0.01 1 0.01", category: "AI", desc: "Chance for AI to wander per spawn iteration")]
	protected float m_AIWanderChance;
	
	[Attribute("0.15", UIWidgets.Slider, params: "0.01 1 0.01", category: "AI", desc: "Minimum percentage of AI to wander")]
	protected float m_AIWanderMinimumPercent;
	
	[Attribute("0.5", UIWidgets.Slider, params: "0.01 1 0.01", category: "AI", desc: "Maximum percentage of AI to wander")]
	protected float m_AIWanderMaximumPercent;
	
	protected ref array<SCR_AIGroup> m_CurrentGroups = {};
	protected ref array<SCR_TW_AISpawnPoint> m_AISpawnPoints = {};
	protected ref array<IEntity> players;
	protected ref array<SCR_TW_VehicleSpawn> m_VehicleSpawnPoints = {};
	protected ref array<IEntity> m_SpawnedVehicles = {};
	protected ref array<SCR_ChimeraAIAgent> m_Groups = {};
	
	static SCR_TW_ExtractionSpawnHandler sInstance;
	
	static SCR_TW_ExtractionSpawnHandler GetInstance()
	{
		if(sInstance) return sInstance;
		
		if(!GetGame().InPlayMode())
			return null;
		
		BaseGameMode gameMode = GetGame().GetGameMode();
		
		if(!gameMode)
			return null;
		
		sInstance = SCR_TW_ExtractionSpawnHandler.Cast(gameMode.FindComponent(SCR_TW_ExtractionSpawnHandler));
		
		return sInstance;
	}
	
	override void OnGameModeStart()
	{		
		if(!TW_Global.IsServer(GetOwner()))
			return;
		
		if(m_DisableSpawns)
			return;
		
		GetGame().GetCallqueue().CallLater(FirstPass, 15 * 1000, false);
		
		// These methods periodically update our stuff. Thus need to repeat indefinitely
		GetGame().GetCallqueue().CallLater(ReinitializePlayers, SCR_TW_Util.FromSecondsToMilliseconds(m_GarbageCollectionTimer), true);
		GetGame().GetCallqueue().CallLater(GarbageCollection, SCR_TW_Util.FromSecondsToMilliseconds(m_GarbageCollectionTimer), true);
		GetGame().GetCallqueue().CallLater(SpawnLoop, SCR_TW_Util.FromMinutesToMilliseconds(m_SpawnTimerInMinutes), true);
		
		Print(string.Format("TrainWreck: Registered Vehicle Spawn Points: %1", m_VehicleSpawnPoints.Count()), LogLevel.WARNING);
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
		
		Print(string.Format("TrainWreck: %1 For GC", agent.ClassName()), LogLevel.WARNING);
		SCR_AIGroup group = SCR_AIGroup.Cast(agent);
		
		if(group)
		{
			auto characters = group.GetAIMembers();
		
			foreach(SCR_ChimeraCharacter character : characters)
				SCR_EntityHelper.DeleteEntityAndChildren(character);
			
			SCR_EntityHelper.DeleteEntityAndChildren(group);
		}
			
		SCR_ChimeraAIAgent ai = SCR_ChimeraAIAgent.Cast(agent);
			
		if(ai)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(agent);
			return;
		}
	}
	
	void ProcessVehicleForGC(IEntity vehicle)
	{
		if(!vehicle) return;
		
		if(!SCR_TW_Util.IsOutsideOfPlayers(vehicle.GetOrigin(), players, m_VehicleGarbageCollectionDistance))
			return;
		
		SCR_EntityHelper.DeleteEntityAndChildren(vehicle);
	}
		
	private void WanderRandomGroups(notnull array<SCR_TW_AISpawnPoint> points)
	{		
		if(m_Groups.IsEmpty())
			return;
		
		int index = m_Groups.GetRandomIndex();
		SCR_ChimeraAIAgent agent = m_Groups.Get(index);
		m_Groups.Remove(index);
		
		SCR_AIInfoComponent aiInfo = TW<SCR_AIInfoComponent>.Find(agent);
		
		if(!aiInfo)
			return;
		
		EAIThreatState state = aiInfo.GetThreatState();
		
		if(state == EAIThreatState.SAFE || SCR_Enum.HasFlag(state, EAIThreatState.SAFE))
		{
			AIGroup group = agent.GetParentGroup();			
			
			if(!group)
				return;
			
			SCR_TW_AISpawnPoint moveToPoint = points.GetRandomElement();
			ResourceName randomWaypointType = moveToPoint.GetRandomWaypoint();
			AIWaypoint waypoint = SCR_TW_Util.CreateWaypointAt(randomWaypointType, moveToPoint.GetOrigin());
			
			if(waypoint)
			{
				AIWaypoint currentWaypoint = group.GetCurrentWaypoint();
				
				// Ensure whatever they're currently doing -- it's overriden 
				if(currentWaypoint)
					group.RemoveWaypoint(currentWaypoint);
				
				group.AddWaypoint(waypoint);
			}
		}
	}
	
	void SpawnLoop()
	{
		if(players.Count() <= 0)
			return;
		
		HandleVehicleSpawning();
		
		ref array<SCR_AIGroup> agents = {};		
		ref array<SCR_TW_AISpawnPoint> nearbyPoints = {};
		int currentAgents = GetAgentCount(agents);
		int nearbyCount = 0;
		
		if(currentAgents > 0)
		{
			nearbyCount = GetPointsInPlayerVicinity(nearbyPoints);
			
			float wanderChance = Math.RandomFloat01();
			
			if(wanderChance <= m_AIWanderChance)
			{
				float wanderPercent = Math.RandomFloatInclusive(m_AIWanderMinimumPercent, m_AIWanderMaximumPercent);
				int wanderCount = Math.Round(m_Groups.Count() * wanderPercent);
				
				for(int i = 0; i < wanderCount; i++)
					WanderRandomGroups(nearbyPoints);
			}			
		}
		
		if(currentAgents >= m_MaxAgents)
		{
			Print(string.Format("TrainWreck: Spawn system is capped at %1/%2", currentAgents, m_MaxAgents), LogLevel.WARNING);
			return;
		}
		else if(nearbyCount == 0)
			nearbyCount = GetPointsInPlayerVicinity(nearbyPoints);		
		
		if(nearbyCount == 0)
		{
			Print("TrainWreck: Failed to find nearby SCR_TW_AISpawnPoints", LogLevel.WARNING);
			return;
		}
		
		int max = Math.Min(10, nearbyCount);
		int diff = m_MaxAgents - currentAgents;
		
		int spawnCount = Math.RandomIntInclusive(0, Math.Min(max, diff));
		
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
			
			// Stagger spawns to prevent lag spike each spawn iteration 
			GetGame().GetCallqueue().CallLater(InvokeSpawnOn, SCR_TW_Util.FromSecondsToMilliseconds(1 * i), false, currentPoint);
		}
	}
	
	private void HandleVehicleSpawning()
	{
		ref array<SCR_TW_VehicleSpawn> nearbyPoints = {};
		int nearbyCount = GetVehiclePointsInPlayerVicinity(nearbyPoints);
		
		if(nearbyCount == 0)
		{
			Print("TrainWreck: No vehicle spawn points nearby", LogLevel.WARNING);
			return;
		}
		else
			Print(string.Format("TrainWreck: Nearby Vehicle Spawns: %1", nearbyCount), LogLevel.WARNING);
		
		int max = Math.Min(6, nearbyCount);
		int spawnCount = Math.RandomIntInclusive(1, max);
		
		int i = 0;
		foreach(SCR_TW_VehicleSpawn point : nearbyPoints)			
		{
			UnregisterVehicleSpawnPoint(point);
			GetGame().GetCallqueue().CallLater(InvokeSpawnOnVehicle, SCR_TW_Util.FromSecondsToMilliseconds(i * 1), false, point);
			i++;
		}
	}
	
	private void InvokeSpawnOnVehicle(SCR_TW_VehicleSpawn spawnPoint)
	{
		IEntity vehicle;
		if(spawnPoint.SpawnVehicle(vehicle))
		{
			if(vehicle)
				m_SpawnedVehicles.Insert(vehicle);
			else
				Print("TrainWreck: Vehicle spawned successfully but variable is null", LogLevel.ERROR);
		}
		else
			Print("TrainWreck: Vehicle did not spawn", LogLevel.DEBUG);
	}
	
	// This is purely to enable delayed/staggered spawning
	private void InvokeSpawnOn(SCR_TW_AISpawnPoint spawnPoint)
	{
		spawnPoint.Spawn();
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
	
	int GetVehiclePointsInPlayerVicinity(notnull array<SCR_TW_VehicleSpawn> points)
	{
		int count = 0;
		foreach(SCR_TW_VehicleSpawn point : m_VehicleSpawnPoints)
		{
			if(!point) continue;
			if(!SCR_TW_Util.IsWithinRange(point.GetOwner().GetOrigin(), players, m_MinimumVehicleSpawnDistance, m_MaximumVehicleSpawnDistance))
				continue;
			
			points.Insert(point);
			count++;
		}
		
		return count;
	}
	
	void GarbageCollection()
	{
		ref array<SCR_AIGroup> agents = {};
		int currentAgents = GetAgentCount(agents);
		int queuedForGC = 0;
		int vehiclesQueuedForGC = 0;
		
		if(currentAgents <= 0 && m_SpawnedVehicles.IsEmpty())
			return;
		
		Print("TrainWreck: GarbageCollection Pass");
		
		foreach(SCR_AIGroup agent : agents)
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
	
	void RegisterVehicleSpawnPoint(SCR_TW_VehicleSpawn point)
	{
		if(!point)
		{
			Print("TrainWreck: Vehicle spawn point cannot be null", LogLevel.ERROR);
			return;
		}
		
		m_VehicleSpawnPoints.Insert(point);
	}
	
	void UnregisterVehicleSpawnPoint(SCR_TW_VehicleSpawn point)
	{
		if(!point)
			return;
		
		m_VehicleSpawnPoints.RemoveItem(point);
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
			
	int GetAgentCount(out notnull array<SCR_AIGroup> agents)
	{
		m_Groups.Clear();
		
		ref array<AIAgent> worldAgents = {};
		GetGame().GetAIWorld().GetAIAgents(worldAgents);
		// We want to ensure we're only grabbing AI we care about -- specifically AI
		foreach(auto agent : worldAgents)
		{
			SCR_AIGroup group = SCR_AIGroup.Cast(agent);
			
			if(group)
				agents.Insert(group);				
			else
			{
				SCR_ChimeraAIAgent aiAgent = SCR_ChimeraAIAgent.Cast(agent);
				if(aiAgent)
					m_Groups.Insert(aiAgent);
				else
					Print(string.Format("TrainWreck: %1 - not counted towards AI pool", agent.ClassName()), LogLevel.WARNING);	
			}				
		}
		
		return agents.Count();
	}
	
};