[EntityEditorProps(category: "GameScripted/TrainWreck/Systems", description: "Event sites for Extraction GameMode")]
class SCR_TW_EventSiteClass : SCR_SiteSlotEntityClass
{
	
};

class SCR_TW_EventSite : SCR_SiteSlotEntity
{
	[Attribute("", UIWidgets.Auto, "Prefabs")]
	private ref array<ResourceName> m_EventPrefabs;
	
	[Attribute("", UIWidgets.ResourceNamePicker, category: "Prefabs")]
	private ResourceName m_TriggerPrefab;
	
	[Attribute("0.2", UIWidgets.Slider, params: "0 1 0.01", category: "Chance", desc: "Chance to roll for site to spawn")]
	private float m_SpawnChance;
	
	//! Has the event site been visited by a player?
	private bool m_HasBeenVisited = false;
	
	//! Trigger to spawn on site
	private SCR_TW_TriggerArea m_Trigger;
	private ResourceName m_SpawnedPrefab;
	private IEntity m_SpawnedEntity;
	
	private ref array<SCR_TW_IntelligenceSpawnerComponent> m_IntelligenceSpawners = {};
	private ref array<SCR_TW_InventoryLoot> m_LootContainers = {};
	private ref array<SCR_TW_EventAISpawner> m_Spawners = {};
	private bool canSpawn;
	
	private bool RollChance() 
	{
		return Math.RandomFloat01() <= m_SpawnChance;
	}
	
	//! Is something loaded on site.
	bool HasBeenLoaded() { return m_SpawnedEntity != null; }
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		if(!TW_Global.IsInRuntime())
			return;
		
		canSpawn = RollChance();
		
		SCR_TW_ExtractionSpawnHandler.GetInstance().RegisterEventSite(this);
	}

	void Despawn()
	{
		if(!canSpawn)
			return;
		
		Print(string.Format("TrainWreck: Despawning Event Site %1", SCR_TW_Util.ToGridText(GetOrigin())));
		
		m_IntelligenceSpawners.Clear();
		m_LootContainers.Clear();
		m_Spawners.Clear();
		
		SCR_EntityHelper.DeleteEntityAndChildren(m_SpawnedEntity);
		
		if(GetOccupant())
			SCR_EntityHelper.DeleteEntityAndChildren(GetOccupant());
		
		SetOccupant(m_SpawnedEntity); // should be null at this point
	}
	
	void SpawnSite()
	{
		if(!canSpawn) return;
		
		Print(string.Format("TrainWreck: Spawning Event Site %1", SCR_TW_Util.ToGridText(GetOrigin())));
		SpawnTrigger();
		
		if(!m_SpawnedPrefab)
			m_SpawnedPrefab = m_EventPrefabs.GetRandomElement();
		
		Resource resource = Resource.Load(m_SpawnedPrefab);
		m_SpawnedEntity = SpawnEntityInSlot(resource);
		
		GetGame().GetCallqueue().CallLater(Scan, SCR_TW_Util.FromSecondsToMilliseconds(1), false);
	}		
	
	private void Scan()
	{				
		// We have to scan this entity regardless because it's new, the old references won't work
		ProcessEntity(m_SpawnedEntity);
		
		Print(string.Format("TrainWreck: EventSite Info: \n\tSpawners: %1\n\tLoot: %2\n\tIntelligence: %3", m_Spawners.Count(), m_LootContainers.Count(), m_IntelligenceSpawners.Count()), LogLevel.WARNING);		
		
		// To avoid spawn lag --> trickle spawn units at event sites
		foreach(int i, SCR_TW_EventAISpawner spawner : m_Spawners)
			GetGame().GetCallqueue().CallLater(TrickleSpawn, i * 250, false, spawner);
		
		// Loot and intelligence shall not spawn if event area
		// has been visited by a player
		
		if(m_HasBeenVisited)
			return;
		
		// Intelligence spawn
		foreach(SCR_TW_IntelligenceSpawnerComponent container : m_IntelligenceSpawners)
			container.SpawnIntelligence();
		
		// Spawn loot
		foreach(int i, SCR_TW_InventoryLoot loot : m_LootContainers)
			GetGame().GetCallqueue().CallLater(TrickleLoot, i * 250, false, loot);	
	}
	
	private void TrickleSpawn(SCR_TW_EventAISpawner spawner)
	{
		spawner.SpawnUnit(m_HasBeenVisited);
	}
	
	private void TrickleLoot(SCR_TW_InventoryLoot loot)
	{
		TW_LootManager.SpawnLootInContainer(loot);
	}		
	
	private void ProcessEntity(IEntity entity)
	{
		if(!entity) return;
		ref array<IEntity> entities = {};
		SCR_EntityHelper.GetHierarchyEntityList(entity, entities);
		
		foreach(IEntity current : entities)
			ProcessComponents(current);
	}
	
	private void ProcessComponents(IEntity entity)
	{
		if(!entity) return;
		
		SCR_TW_InventoryLoot lootItem = TW<SCR_TW_InventoryLoot>.Find(entity);
		
		if(lootItem)
			m_LootContainers.Insert(lootItem);
		
		SCR_TW_IntelligenceSpawnerComponent intelligence = TW<SCR_TW_IntelligenceSpawnerComponent>.Find(entity);
		
		if(intelligence)
			m_IntelligenceSpawners.Insert(intelligence);
		
		SCR_TW_EventAISpawner spawner = SCR_TW_EventAISpawner.Cast(entity);
		
		if(spawner)
			m_Spawners.Insert(spawner);
	}
	
	private void SpawnTrigger()
	{
		if(m_Trigger) 
			return;
		
		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;
		GetTransform(params.Transform);				
		
		vector position = GetOrigin();
		position[1] = GetWorld().GetSurfaceY(position[0], position[2]);
		params.Transform[3] = position;
		
		Resource resource = Resource.Load(m_TriggerPrefab);
		
		if(!resource.IsValid())
			return;
		
		IEntity entity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);
		m_Trigger = SCR_TW_TriggerArea.Cast(entity);
		
		m_Trigger.OnTriggered.Insert(OnEntered);		
	}
	
	void ~SCR_TW_EventSite()
	{
		if(m_Trigger)
			m_Trigger.OnTriggered.Remove(OnEntered);
	}
	
	//! Triggers visited site to be true. 
	private void OnEntered()
	{
		m_HasBeenVisited = true;
	}
};