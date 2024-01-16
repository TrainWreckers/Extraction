class SCR_TW_ExtractionSpawnHandlerClass : SCR_BaseGameModeComponentClass {};
class SCR_TW_ExtractionSpawnHandler : SCR_BaseGameModeComponent
{
	[Attribute("", UIWidgets.CheckBox, category: "Spawn Details")]
	protected bool m_DisableSpawns;
	
	[Attribute("10", UIWidgets.Slider, params: "5 75 1", category: "Spawn Details")]
	protected int m_MaxAgents;
	
	[Attribute("2", UIWidgets.Slider, params: "1 300 1", category: "Spawn Details", desc: "Time in seconds for checking when to spawn AI")]
	protected int m_SpawnTimerInSeconds;
	
	[Attribute("1", UIWidgets.Slider, params: "1 10 1", category: "Spawn Details", desc: "Chunks around players (1000m) that should be active")]
	protected int m_SpawnGridRadius;
	
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
	
	[Attribute("750", UIWidgets.Slider, params: "100 2000 25", category: "Events", desc: "Distance players must be within for event sites to activate")]
	private int m_EventSiteActivationDistance;
	
	[Attribute("800", UIWidgets.Slider, params: "100 2000 25", category: "Events", desc: "Distance players must be out of for event sites to despawn")]
	private int m_EventSiteDespawnDistance;
	
	[Attribute("10", UIWidgets.Slider, params: "1 300 1", category: "Events", desc: "Time in seconds for checking event areas")]
	private int m_EventCheckTimer;
	
	private ref array<SCR_TW_EventSite> eventSites = {};
	protected ref array<SCR_AIGroup> m_CurrentGroups = {};
	protected ref array<SCR_TW_AISpawnPoint> m_AISpawnPoints = {};
	protected ref array<IEntity> players = {};
	protected ref array<SCR_TW_VehicleSpawn> m_VehicleSpawnPoints = {};
	protected ref array<IEntity> m_SpawnedVehicles = {};
	protected ref array<SCR_ChimeraAIAgent> m_Groups = {};
	
	protected ref TW_GridCoordManager<SCR_TW_AISpawnPoint> spawnGrid = new TW_GridCoordManager<SCR_TW_AISpawnPoint>();
	protected ref TW_GridCoordManager<SCR_TW_EventSite> eventGrid = new TW_GridCoordManager<SCR_TW_EventSite>();
	protected ref TW_GridCoordManager<SCR_TW_VehicleSpawn> vehicleGrid = new TW_GridCoordManager<SCR_TW_VehicleSpawn>();
	
	protected ref array<ref TW_GridCoord<SCR_TW_AISpawnPoint>> spawnPointsNearPlayers = {};
	protected ref array<SCR_TW_EventSite> eventSitesNearPlayers = {};
	protected ref array<SCR_TW_VehicleSpawn> vehicleSpawnsNearPlayers = {};
	protected int playerChunkCount = 0;
	protected ref set<string> playerChunks = new set<string>();
	
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
		GetGame().GetCallqueue().CallLater(ReinitializePlayers, SCR_TW_Util.FromSecondsToMilliseconds(10), true);
		GetGame().GetCallqueue().CallLater(GarbageCollection, SCR_TW_Util.FromSecondsToMilliseconds(m_GarbageCollectionTimer), true);
		GetGame().GetCallqueue().CallLater(SpawnLoop, SCR_TW_Util.FromSecondsToMilliseconds(m_SpawnTimerInSeconds), true);
		
		Print(string.Format("TrainWreck: Registered Vehicle Spawn Points: %1", m_VehicleSpawnPoints.Count()), LogLevel.WARNING);
	}
	
	void RegisterEventSite(SCR_TW_EventSite site)
	{
		eventGrid.InsertByWorld(site.GetOrigin(), site);
	}
	
	void UnregisterEventSite(SCR_TW_EventSite site)
	{
		Debug.Error("Implement this");
		// eventSites.RemoveItem(site);
	}
	
	void FirstPass()
	{
		ReinitializePlayers();
	}
	
	void ProcessForGC(AIAgent agent)
	{
		if(!agent) return;
		
		if(!SCR_TW_Util.IsOutsideOfPlayers(agent.GetOrigin(), players, m_MaximumAIDistance))
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
		
	private void WanderRandomGroups()
	{		
		if(m_Groups.IsEmpty())
			return;				
		
		int index = m_Groups.GetRandomIndex();
		SCR_ChimeraAIAgent agent = m_Groups.Get(index);
		m_Groups.Remove(index);
		
		// Only wander if they aren't flagged to ignore wandering system.
		if(!SCR_TW_Util.IsValidWanderer_Agent(agent))
			return;
		
		SCR_AIInfoComponent aiInfo = TW<SCR_AIInfoComponent>.Find(agent);
		
		if(!aiInfo)
			return;
		
		EAIThreatState state = aiInfo.GetThreatState();
		
		if(state == EAIThreatState.SAFE || SCR_Enum.HasFlag(state, EAIThreatState.SAFE))
		{
			AIGroup group = agent.GetParentGroup();			
			
			if(!group)
				return;
			
			SCR_TW_AISpawnPoint moveToPoint = spawnGrid.GetNextItemFromPointer(playerChunks); // spawnPointsNearPlayers.GetRandomElement(); points.GetRandomElement();
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
	
	private int spawnQueueCount = 0;
	private int currentAgents = 0;
	
	void SpawnLoop()
	{
		if(!TW_Global.IsServer(GetOwner()))
			return;
		
		if(players.Count() <= 0)
			return;
		
		ref array<SCR_AIGroup> agents = {};		
		currentAgents = GetAgentCount(agents);
		
		// Wandering system
		if(currentAgents > 0)
		{
			float wanderChance = Math.RandomFloat01();
			
			if(wanderChance <= m_AIWanderChance)
			{
				float wanderPercent = Math.RandomFloatInclusive(m_AIWanderMinimumPercent, m_AIWanderMaximumPercent);
				int wanderCount = Math.Round(m_Groups.Count() * wanderPercent);
				
				for(int i = 0; i < wanderCount; i++)
					WanderRandomGroups();
			}
		}
		
		if(spawnQueueCount > 0)
		{
			Print(string.Format("TrainWreck: Skipping spawn. %1 are scheduled for spawning", spawnQueueCount), LogLevel.WARNING);
			return;
		}
		
		if(currentAgents >= m_MaxAgents)
		{
			Print(string.Format("TrainWreck: Spawn system is capped at %1/%2", currentAgents, m_MaxAgents), LogLevel.WARNING);
			return;
		}
		
		int diff = m_MaxAgents - currentAgents;
		
		int spawnCount = Math.RandomIntInclusive(0, Math.Min(10, diff));
		spawnQueueCount += spawnCount;
		
		Print(string.Format("TrainWreck: SpawnLoop(SC: %1, Queued: %2) - Agents: %3", spawnCount, spawnQueueCount, currentAgents), LogLevel.ERROR); 
		
		for(int i = 0; i < spawnCount; i++)
		{
			if(currentAgents >= m_MaxAgents) 
				break;
			
			SCR_TW_AISpawnPoint currentPoint = spawnGrid.GetNextItemFromPointer(playerChunks);
			
			if(!currentPoint.CanSpawn())
				continue;
			
			// Stagger spawns to prevent lag spike each spawn iteration 
			GetGame().GetCallqueue().CallLater(InvokeSpawnOn, SCR_TW_Util.FromSecondsToMilliseconds(0.15 * i), false, currentPoint);
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
		if(currentAgents > m_MaxAgents || !spawnPoint.CanSpawn())
			return;
		
		spawnQueueCount = Math.ClampInt(spawnQueueCount - 1, 0, int.MAX);
		
		// Don't bother spawning if this isn't within acceptable range
		if(!SCR_TW_Util.IsWithinRange(spawnPoint.GetOrigin(), players, m_MinimumAISpawnDistance, m_MaximumAIDistance))
			return;
		
		Print(string.Format("TrainWreck: Spawn Coord(%1)", spawnPoint.GetOrigin()));		
		SCR_AIGroup group = spawnPoint.Spawn();				
	}
	
	void GarbageCollection()
	{
		ref array<SCR_AIGroup> agents = {};
		int currentAgents = GetAgentCount(agents, true);
		int queuedForGC = 0;
		int vehiclesQueuedForGC = 0;
		
		if(currentAgents <= 0 && m_SpawnedVehicles.IsEmpty())
			return;
		
		Print("TrainWreck: GarbageCollection Pass");
		
		int x, y;
		foreach(SCR_AIGroup agent : agents)
		{
			if(!agent)
				continue;
			
			// If the chunk is not loaded --> delete 
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
		
		bool positionsHaveChanged = false;
		ref set<string> currentPositions = new set<string>();
		
		foreach(int playerId : playerIds)
		{
			auto player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			
			if(!player)
				continue;
			
			int x, y;
			SCR_TW_Util.ToGrid(player.GetOrigin(), x, y);
			string position = string.Format("%1 %2", x, y);
			
			if(!currentPositions.Contains(position))
				currentPositions.Insert(position);
			
			players.Insert(player);
		}
		
		ref set<string> unloaded = new set<string>();
		
		// are previous positions still valid?
		foreach(string previous : playerChunks)
			if(!currentPositions.Contains(previous))
			{
				positionsHaveChanged = true;
				playerChunks.RemoveItem(previous);
				unloaded.Insert(previous);
				
				// We will need to despawn sites in these unloaded chunks
				foreach(SCR_TW_EventSite site : eventSitesNearPlayers)
				{
					int sx, sy;
					SCR_TW_Util.ToGrid(site.GetOrigin(), sx, sy);
					string sp = string.Format("%1 %2", sx, sy);
					
					if(unloaded.Contains(sp))
						site.Despawn();
				}
			}
		
		// are any current positions new?
		foreach(string current : currentPositions)
			if(!playerChunks.Contains(current))
			{
				playerChunks.Insert(current);
				positionsHaveChanged = true;		
			}
		
		playerChunkCount = playerChunks.Count();
		
		// If positions have changed we need to regrab all nearby spawn positions
		if(positionsHaveChanged)
		{
			Print("TrainWreck: Player Grid Squares changed since last check. Updating spawn areas");
			spawnPointsNearPlayers.Clear();
			eventSitesNearPlayers.Clear();
			vehicleSpawnsNearPlayers.Clear();
			
			int spawnPointCount = spawnGrid.GetChunksAround(spawnPointsNearPlayers, playerChunks, m_SpawnGridRadius);
			int eventPointCount = eventGrid.GetNeighborsAround(eventSitesNearPlayers, playerChunks, m_SpawnGridRadius);			
			int vehiclePointCount = vehicleGrid.GetNeighborsAround(vehicleSpawnsNearPlayers, playerChunks, m_SpawnGridRadius);
			
			// Spawn vehicles in loaded chunks
			IEntity vehicle;
			int vehicleSpawnCount = Math.RandomIntInclusive(1, vehiclePointCount);
			
			for(int i = 0; i < vehicleSpawnCount; i++)
			{
				int vehicleIndex = vehicleSpawnsNearPlayers.GetRandomIndex();
				SCR_TW_VehicleSpawn vehicleSpawn = vehicleSpawnsNearPlayers.Get(vehicleIndex);				
				GetGame().GetCallqueue().CallLater(InvokeSpawnOnVehicle, SCR_TW_Util.FromSecondsToMilliseconds(i * 1), false, vehicleSpawn);	
				vehicleSpawnsNearPlayers.Remove(vehicleIndex);
			}
			
			// Cleanup vehicle spawns
			vehicleSpawnsNearPlayers.Clear();
			
			// Vehicles only spawn once, no respawn. So we can clean up as we spawn
			vehicleGrid.RemoveCoords(playerChunks);
			
			foreach(SCR_TW_EventSite site : eventSitesNearPlayers)
				if(!site.IsOccupied())	
					site.SpawnSite();
			
			Print(string.Format("TrainWreck: found %1 spawn points in player vicinity. %2 Event sites nearby", spawnPointCount, eventPointCount));			
		}
		
		this.players = players;
	}
	
	void RegisterVehicleSpawnPoint(SCR_TW_VehicleSpawn point)
	{
		if(!point)
		{
			Print("TrainWreck: Vehicle spawn point cannot be null", LogLevel.ERROR);
			return;
		}
		
		vehicleGrid.InsertByWorld(point.GetOwner().GetOrigin(), point);
	}
	
	void UnregisterVehicleSpawnPoint(SCR_TW_VehicleSpawn point)
	{
		if(!point)
			return;
		Debug.Error("Implement");		
	}
	
	void RegisterAISpawnPoint(SCR_TW_AISpawnPoint point)
	{
		if(!point)
		{
			Print("TrainWreck: invalid spawn point. Cannot be null", LogLevel.ERROR);
			return;
		}
		
		spawnGrid.InsertByWorld(point.GetOrigin(), point);
	}
	
	void UnregisterAISpawnPoint(SCR_TW_AISpawnPoint point)
	{
		if(!point) return;
		Debug.Error("Need to implement");
		//m_AISpawnPoints.RemoveItem(point);
	}
			
	int GetAgentCount(out notnull array<SCR_AIGroup> agents, bool includeAll = false)
	{
		m_Groups.Clear();
		
		ref array<AIAgent> worldAgents = {};
		GetGame().GetAIWorld().GetAIAgents(worldAgents);
		// We want to ensure we're only grabbing AI we care about -- specifically AI
		foreach(auto agent : worldAgents)
		{
			SCR_AIGroup group = SCR_AIGroup.Cast(agent);
			
			if(group)
			{
				if(includeAll || !group.IgnoreGlobalCount())
					agents.Insert(group);
			}
				
			else
			{
				SCR_ChimeraAIAgent aiAgent = SCR_ChimeraAIAgent.Cast(agent);
				if(aiAgent)
					m_Groups.Insert(aiAgent);
			}				
		}
		
		return agents.Count();
	}
	
};