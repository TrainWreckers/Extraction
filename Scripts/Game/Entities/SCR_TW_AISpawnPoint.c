[EntityEditorProps(category: "GameScripted/TrainWreck/Systems", description: "Spawn point for units, and behaves as waypoints in the extraction gamemode.")]
class SCR_TW_AISpawnPointClass : GenericEntityClass{};

class SCR_TW_AISpawnPoint : GenericEntity
{
	[Attribute("", UIWidgets.ResourceAssignArray, params: "et", category: "Spawn Info")]
	protected ref array<ResourceName> m_EntityPrefabs;
	
	[Attribute("1", UIWidgets.CheckBox, category: "Spawn Info")]
	protected bool m_SpawnOnPoint;
	
	[Attribute("5", UIWidgets.Slider, params: "0 100 1", category: "Spawn Info")]
	protected float m_SpawnRadius;
	
	[Attribute("50", UIWidgets.Slider, params: "0.1 100 0.1", category: "Spawn Info")]
	protected float m_ChanceToSpawn;
	
	[Attribute("0", UIWidgets.CheckBox, category: "Spawn Info", desc: "Can units respawn here later?")]
	protected bool m_CanRespawn;
	
	[Attribute("", UIWidgets.ResourceAssignArray, params: "et", category: "Spawn Info")]
	protected ref array<ResourceName> m_WaypointPrefabs;
	
	private bool m_HasSpawned = false;
	
	void SCR_TW_AISpawnPoint(IEntitySource src, IEntity parent)
	{
		SetEventMask(EntityEvent.INIT);
		SetFlags(EntityFlags.ACTIVE, true);
	}
	
	override void EOnInit(IEntity owner)
	{
		if(!GetGame().InPlayMode())
			return;
		
		RplComponent rpl = RplComponent.Cast(FindComponent(RplComponent));
		
		if(!rpl.IsMaster())
			return;
		
		SCR_TW_ExtractionSpawnHandler spawnHandler = SCR_TW_ExtractionSpawnHandler.GetInstance();
		
		if(!spawnHandler)
		{
			Print("TrainWreck: SCR_TW_AISpawnPoint requires an instance of SCR_TW_ExtractionSpawnHandler", LogLevel.ERROR);
			return;
		}
		
		spawnHandler.RegisterAISpawnPoint(this);
	}
		
	bool ChanceToSpawn() { return Math.RandomFloat(0, 100) <= m_ChanceToSpawn; }
	float GetSpawnRadius() { return m_SpawnRadius; }
	bool CanSpawnInRadius() { return !m_SpawnOnPoint; }
	
	bool CanSpawn()
	{
		// Can respawn OR hasn't spawned in and can't respawn
		return m_CanRespawn || (!m_CanRespawn && !m_HasSpawned);
	}
	
	ResourceName GetRandomPrefab() { return m_EntityPrefabs.GetRandomElement(); }
	ResourceName GetRandomWaypoint() { return m_WaypointPrefabs.GetRandomElement(); }
	
	SCR_AIGroup Spawn()
	{
		if(!TW_Global.IsServer(this))
			return null;
		
		vector spawnPosition;
		
		if(CanSpawnInRadius())
			spawnPosition = SCR_TW_Util.RandomPositionAround(this, GetSpawnRadius());
		else
		{			
			vector mat[4];
			GetWorldTransform(mat);
			spawnPosition = mat[3];
		}
		
		auto prefab = GetRandomPrefab();		
		SCR_AIGroup group = SCR_TW_Util.SpawnGroup(prefab, spawnPosition, 1);
		
		if(!group)
		{
			Print("TrainWreck: Was unable to successfully spawn group", LogLevel.ERROR);
			return null;
		}
		
		auto waypointPrefab = GetRandomWaypoint();
		AIWaypoint waypoint = SCR_TW_Util.CreateWaypointAt(waypointPrefab, spawnPosition);
		
		if(!waypoint)
		{
			Print("TrainWreck: Invalid waypoint provided.", LogLevel.ERROR);
			return null;
		}
		
		group.AddWaypoint(waypoint);
		return group;
	}
};