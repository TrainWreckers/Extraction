class SCR_TW_ExtractionSpawnHandlerClass : SCR_BaseGameModeComponentClass {};
class SCR_TW_ExtractionSpawnHandler : SCR_BaseGameModeComponent
{
	[Attribute("", UIWidgets.CheckBox, category: "Spawn Details")]
	protected bool m_DisableSpawns;
	
	[Attribute("", UIWidgets.CheckBox, category: "Spawn Details")]
	protected bool m_DisableEvents;
	
	[Attribute("10", UIWidgets.Slider, params: "5 75 1", category: "Spawn Details")]
	protected int m_MaxAgents;
	
	[Attribute("2", UIWidgets.Slider, params: "1 300 1", category: "Spawn Details", desc: "Time in seconds for checking when to spawn AI")]
	protected int m_SpawnTimerInSeconds;
	
	[Attribute("1", UIWidgets.Slider, params: "1 10 1", category: "Spawn Details", desc: "Chunks around players (1000m) that should be active")]
	protected int m_SpawnGridRadius;
	
	[Attribute("500", UIWidgets.Slider, params: "100 5000 100", category: "Spawn Details", desc: "Size of grid for spawning AI")]
	protected int m_SpawnGridSize;
	
	[Attribute("1", UIWidgets.Slider, params: "1 20 1", category: "Spawn Details", desc: "Chunks around players (AntiSpawnGridSize) where AI cannot spawn")]
	protected int m_AntiSpawnGridRadius;
	
	[Attribute("100", UIWidgets.Slider, params: "25 1000 25", category: "Spawn Details", desc: "Grid size to use for Anti Spawn System")]
	protected int m_AntiSpawnGridSize;
	
	[Attribute("120", UIWidgets.Slider, params: "0 500 1", category: "Garbage Collection", desc: "Time in seconds. Interval GC is checked (nothing happens)")]
	protected float m_GarbageCollectionTimer;
	
	[Attribute("0.6", UIWidgets.Slider, params: "0.00 1 0.01", category: "AI", desc: "Chance for AI to wander per spawn iteration")]
	protected float m_AIWanderChance;
	
	[Attribute("0.15", UIWidgets.Slider, params: "0.01 1 0.01", category: "AI", desc: "Minimum percentage of AI to wander")]
	protected float m_AIWanderMinimumPercent;
	
	[Attribute("0.5", UIWidgets.Slider, params: "0.01 1 0.01", category: "AI", desc: "Maximum percentage of AI to wander")]
	protected float m_AIWanderMaximumPercent;
	
	[Attribute("{6D56FED1E55A8F84}Prefabs/Items/Misc/IntelligenceFolder_E_01/IntelligenceFolder_E_01.et", UIWidgets.ResourcePickerThumbnail, category: "Rewards", desc: "Intelligence reward", params: "et")]
	protected ResourceName m_IntelligenceRewardPrefab;
	
	[Attribute("", UIWidgets.Auto, category: "Wandering Waypoints", desc: "These will be assigned in order to wandering units", params: "et")]
	protected ref array<ResourceName> m_WanderingWaypoints;
	
	private ref array<SCR_TW_EventSite> eventSites = {};
	protected ref array<SCR_AIGroup> m_CurrentGroups = {};
	protected ref array<SCR_TW_AISpawnPoint> m_AISpawnPoints = {};
	protected ref array<IEntity> players = {};
	protected ref array<SCR_TW_VehicleSpawn> m_VehicleSpawnPoints = {};
	protected ref array<IEntity> m_SpawnedVehicles = {};
	protected ref array<SCR_ChimeraAIAgent> m_Groups = {};
	
	protected ref TW_GridCoordManager<SCR_TW_AISpawnPoint> spawnGrid = new TW_GridCoordManager<SCR_TW_AISpawnPoint>(m_SpawnGridSize);
	protected ref TW_GridCoordManager<SCR_TW_EventSite> eventGrid = new TW_GridCoordManager<SCR_TW_EventSite>(m_SpawnGridSize);
	protected ref TW_GridCoordManager<SCR_TW_VehicleSpawn> vehicleGrid = new TW_GridCoordManager<SCR_TW_VehicleSpawn>(m_SpawnGridSize);
	
	protected ref array<ref TW_GridCoord<SCR_TW_AISpawnPoint>> spawnPointsNearPlayers = {};
	protected ref array<SCR_TW_AISpawnPoint> aiSpawnPointsNearPlayers = {};
	protected ref array<SCR_TW_EventSite> eventSitesNearPlayers = {};
	protected ref array<SCR_TW_VehicleSpawn> vehicleSpawnsNearPlayers = {};
	protected int playerChunkCount = 0;
	protected ref set<string> playerChunks = new set<string>();
	protected ref set<string> antiSpawnChunks = new set<string>();
	
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
	
	ResourceName GetIntelligenceRewardPrefab() { return m_IntelligenceRewardPrefab; }
	
	array<IEntity> GetPlayers() { return players; }
	
	override void OnGameModeStart()
	{		
		if(!TW_Global.IsServer(GetOwner()))
			return;
		
		// These methods periodically update our stuff. Thus need to repeat indefinitely
		// If units can spawn OR events can spawn
		if(!m_DisableSpawns || !m_DisableEvents)
		{
			GetGame().GetCallqueue().CallLater(ReinitializePlayers, SCR_TW_Util.FromSecondsToMilliseconds(10), true);
			GetGame().GetCallqueue().CallLater(GarbageCollection, SCR_TW_Util.FromSecondsToMilliseconds(m_GarbageCollectionTimer), true);
		}
		
		if(!m_DisableSpawns)
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
	}
	
	IEntity GetLocationBySpawnRule(TW_MissionSpawnArea rule)
	{
		switch(rule)
		{
			case TW_MissionSpawnArea.EventSite:
			{
				ref array<SCR_TW_EventSite> allSites = {};
				int siteCount = eventGrid.GetAllItems(allSites);
								
				while(siteCount > 0)
				{
					int index = allSites.GetRandomIndex();
					SCR_TW_EventSite site = allSites.Get(index);
					allSites.Remove(index);
					siteCount--;
					
					if(!site)
						continue;
							
					if(!site.IsOccupied())
						return site;
				}
				
				return null;
			}
			
			case TW_MissionSpawnArea.ExtractionSite:
			{
				return SCR_TW_ExtractionHandler.GetInstance().GetRegisteredExtractionSite();
			}			
		}		
		return null;
	}
	
	void FirstPass()
	{
		ReinitializePlayers();
	}
	
	void ProcessForGC(AIAgent agent)
	{
		if(!agent) 
			return;
		
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
	
	private void WanderRandomGroups()
	{		
		if(m_Groups.IsEmpty())
			return;				
		
		int index = m_Groups.GetRandomIndex();
		SCR_ChimeraAIAgent agent = m_Groups.Get(index);
		
		// Only wander if they aren't flagged to ignore wandering system.
		if(!SCR_TW_Util.IsValidWanderer_Agent(agent))
			return;
		Print("TrainWreck: Wandering 1 agent");
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
			
			// We need to remove waypoints from the group to ensure they'll go to the next set of waypoints
			ref array<AIWaypoint> currentWaypoints = {};
			group.GetWaypoints(currentWaypoints);
			
			foreach(AIWaypoint waypoint : currentWaypoints)
				group.RemoveWaypoint(waypoint);
			
			// Where are they going
			SCR_TW_AISpawnPoint moveToPoint = spawnGrid.GetNextItemFromPointer(playerChunks);
			
			if(!moveToPoint)
				return;
			
			// Add all the wandering waypoints in order of the array
			foreach(ResourceName waypointPrefab : m_WanderingWaypoints)
			{
				AIWaypoint waypoint = SCR_TW_Util.CreateWaypointAt(waypointPrefab, moveToPoint.GetOrigin());
				if(waypoint)
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
			
			if(wanderChance < m_AIWanderChance)
			{
				float wanderPercent = Math.RandomFloatInclusive(m_AIWanderMinimumPercent, m_AIWanderMaximumPercent);
				int wanderCount = Math.Round(m_Groups.Count() * wanderPercent);
				
				for(int i = 0; i < wanderCount; i++)
					WanderRandomGroups();
			}
		}
		
		if(isTrickleSpawning)
		{
			Print("TrainWreck: Skipping spawn. Units are still trickle spawning", LogLevel.WARNING);
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
		
		if(spawnCount > 0)
			isTrickleSpawning = true;
		
		GetGame().GetCallqueue().CallLater(InvokeSpawnOn, 0.15, false, spawnCount);
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
	
	private bool isTrickleSpawning = false;
	
	//! Is the spawn point too close to a player?
	private bool IsValidSpawn(SCR_TW_AISpawnPoint spawnPoint)
	{
		if(!spawnPoint) return false;
		string position = SCR_TW_Util.ToGridText(spawnPoint.GetOrigin(), m_AntiSpawnGridSize);
		return !antiSpawnChunks.Contains(position);
	}
	
	// This is purely to enable delayed/staggered spawning
	private void InvokeSpawnOn(int remainingCount)
	{
		if(remainingCount <= 0)
		{
			isTrickleSpawning = false;
			return;
		}
		else 
			remainingCount -= 1;
		
		if(currentAgents > m_MaxAgents)
			return;
		
		SCR_TW_AISpawnPoint spawnPoint = spawnGrid.GetNextItemFromPointer(playerChunks);
		
		if(IsValidSpawn(spawnPoint))
			SCR_AIGroup group = spawnPoint.Spawn();
		
		// Recurse
		if(remainingCount > 0)
			GetGame().GetCallqueue().CallLater(InvokeSpawnOn, 150, false, remainingCount);
		else 
			isTrickleSpawning = false;
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
		string positionText;
		foreach(int agentN, SCR_AIGroup agent : agents)
		{
			if(!agent)
				continue;
			
			positionText = SCR_TW_Util.ToGridText(agent.GetOrigin(), m_SpawnGridSize);
			
			// If the chunk is not loaded --> delete 
			if(!playerChunks.Contains(positionText))
			{
				GetGame().GetCallqueue().CallLater(ProcessForGC, agentN * 150, false, agent);
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
		
		ref set<string> currentPositions = new set<string>();
		ref set<string> unloaded = new set<string>();
		
		bool positionsHaveChanged = false;
		
		antiSpawnChunks.Clear();
		
		ref set<string> chunksAroundPlayer = new set<string>();
		// Where are players at currently?
		foreach(int playerId : playerIds)
		{
			auto player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			
			if(!player)
				continue;
						
			chunksAroundPlayer.Clear();
			SCR_TW_Util.AddSurroundingGridSquares(antiSpawnChunks, player.GetOrigin(), m_AntiSpawnGridRadius, m_AntiSpawnGridSize);
			SCR_TW_Util.AddSurroundingGridSquares(chunksAroundPlayer, player.GetOrigin(), m_SpawnGridRadius, m_SpawnGridSize);			
			
			foreach(string chunk : chunksAroundPlayer)
			{
				if(!playerChunks.Contains(chunk))
				{
					positionsHaveChanged = true;
					playerChunks.Insert(chunk);
				}
				
				if(!currentPositions.Contains(chunk))
					currentPositions.Insert(chunk);
			}
				
			players.Insert(player);
		}
		
		// are previous positions still valid?
		int ex, ey;
		ref array<SCR_TW_EventSite> coordSites = {};
		
		playerChunkCount = playerChunks.Count();
		
		for(int i = 0; i < playerChunkCount; i++)
		{
			string playerCoord = playerChunks.Get(i);
			
			if(!currentPositions.Contains(playerCoord))
			{
				unloaded.Insert(playerCoord);
				playerChunks.Remove(i);
				
				// Must remember to decrement, otherwise we skip an item
				i -= 1;
				playerChunkCount -= 1;
				
				positionsHaveChanged = true;
				
				coordSites.Clear();
				SCR_TW_Util.FromGridString(playerCoord, ex, ey);
				
				if(eventGrid.HasCoord(ex, ey))
				{
					TW_GridCoord<SCR_TW_EventSite> grid = eventGrid.GetCoord(ex, ey);
					
					if(!grid) continue;
					
					int siteCount = grid.GetData(coordSites);
					foreach(SCR_TW_EventSite site : coordSites)
						if(!site.IsIgnoreGC()) // Only delete those who shouldn't get deleted
							site.Despawn();
				}				
			}
		}
		
		playerChunkCount = playerChunks.Count();
		
		// If positions have changed we need to regrab all nearby spawn positions
		if(positionsHaveChanged)
		{
			Print("TrainWreck: Player Grid Squares changed since last check. Updating spawn areas");
			spawnPointsNearPlayers.Clear();
			eventSitesNearPlayers.Clear();
			vehicleSpawnsNearPlayers.Clear();
			aiSpawnPointsNearPlayers.Clear();
			
			int spawnPointCount = spawnGrid.GetChunksAround(spawnPointsNearPlayers, playerChunks, m_SpawnGridRadius);
			int eventPointCount = eventGrid.GetNeighborsAround(eventSitesNearPlayers, playerChunks, m_SpawnGridRadius);			
			int vehiclePointCount = vehicleGrid.GetNeighborsAround(vehicleSpawnsNearPlayers, playerChunks, m_SpawnGridRadius);
			
			foreach(ref TW_GridCoord<SCR_TW_AISpawnPoint> coord : spawnPointsNearPlayers)
				aiSpawnPointsNearPlayers.InsertAll(coord.GetAll());
			
			if(vehiclePointCount > 0)
			{
				// Spawn vehicles in loaded chunks
				IEntity vehicle;
				
				for(int i = 0; i < vehiclePointCount; i++)
				{
					SCR_TW_VehicleSpawn vehicleSpawn = vehicleSpawnsNearPlayers.Get(i);					
					GetGame().GetCallqueue().CallLater(InvokeSpawnOnVehicle, SCR_TW_Util.FromSecondsToMilliseconds(i * 1), false, vehicleSpawn);
				}
			}
			else Print("TrainWreck: No vehicle spawns were found in the active grid squares", LogLevel.WARNING);
			
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
				if(group.GetPlayerCount() > 0)
					continue;

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
	
	#ifdef WORKBENCH
	
	[Attribute(defvalue: "1", desc: "Show the debug shapes in Workbench", category: "Debug")]
	protected bool m_ShowDebugShapesInWorkbench;
	protected WorldEditorAPI m_API;
	
	protected IEntity m_PreviewEntity;
	
	protected ref Shape m_DebugAISpawnArea;
	protected ref Shape m_DebugAntiSpawnArea;
	
	protected int m_DebugAISpawnAreaColor = Color.GREEN;
	protected int m_DebugAntiSpawnAreaColor = Color.RED;	
	
	protected int previousAntiSpawnRadius, previousAISpawnRadius, previousAIGridSize, previousAntiSpawnGridSize;
	int shapeFlags = ShapeFlags.WIREFRAME | ShapeFlags.VISIBLE;
	
	protected vector GetMinsFor(vector center, int gridSize, int gridRadius)
	{	
		int length = gridSize * (gridRadius * 2);
		return Vector(center[0] - length, 0, center[2] - length);
	}
	
	protected vector GetMaxsFor(vector center, int gridSize, int gridRadius)
	{
		int length = gridSize * (gridRadius * 2);
		return Vector(center[0] + length, center[1] + 100, center[2] + length);
	}
	
	private void CreateAISpawnArea()
	{
		vector origin = GetOwner().GetOrigin();
		m_DebugAISpawnArea = Shape.Create(ShapeType.BBOX, m_DebugAISpawnAreaColor, shapeFlags, 			
			GetMinsFor(origin, m_SpawnGridSize, m_SpawnGridRadius),
			GetMaxsFor(origin, m_SpawnGridSize, m_SpawnGridRadius)
		);
		
		previousAISpawnRadius = m_SpawnGridRadius;
		previousAIGridSize = m_SpawnGridSize;
	}
	
	private void CreateAntiSpawnArea()
	{
		vector origin = GetOwner().GetOrigin();
		
		m_DebugAntiSpawnArea = Shape.Create(ShapeType.BBOX, m_DebugAntiSpawnAreaColor, shapeFlags,
			GetMinsFor(origin, m_AntiSpawnGridSize, m_AntiSpawnGridRadius),
			GetMaxsFor(origin, m_AntiSpawnGridSize, m_AntiSpawnGridRadius)
		);		
		
		previousAntiSpawnRadius = m_AntiSpawnGridRadius;
		previousAntiSpawnGridSize = m_AntiSpawnGridSize;
	}
	
	protected void DrawDebugShape()
	{
		if(!m_ShowDebugShapesInWorkbench)
			return;
		
		CreateAISpawnArea();
		CreateAntiSpawnArea();
		
		/*vector globalTransform[4];
		GetOwner().GetTransform(globalTransform);
		m_DebugShape.SetMatrix(globalTransform);*/
	}
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if(m_ShowDebugShapesInWorkbench)
		{
			if(previousAIGridSize != m_SpawnGridSize ||
				previousAISpawnRadius != m_SpawnGridRadius)
				CreateAISpawnArea();
			
			if(previousAntiSpawnRadius != m_AntiSpawnGridRadius || 
				previousAntiSpawnGridSize != m_AntiSpawnGridSize)
				CreateAntiSpawnArea();
			
			vector transform[4];
		    GetOwner().GetTransform(transform);
		    m_DebugAISpawnArea.SetMatrix(transform);
			m_DebugAntiSpawnArea.SetMatrix(transform);	
		}
	}
	
	override void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		DrawDebugShape();
	}
	
	override bool _WB_OnKeyChanged(IEntity owner, BaseContainer src, string key, BaseContainerList ownerContainers, IEntity parent)
	{
		if(key == "m_ShowDebugShapesInWorkbench")
			DrawDebugShape();
		
		return true;
	}
	
	#endif	
};