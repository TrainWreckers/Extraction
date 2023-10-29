[EntityEditorProps(category: "GameScripted/TrainWreck/Systems", description: "Entity that takes care of managing composition spawning.", color: "0 0 255 255")]
class SCR_TW_CompositionHandlerClass: SCR_BaseGameModeComponentClass
{
	
};

enum SCR_TW_SlotType
{
	SiteSmall,
	SiteMedium,
	SiteLarge,
	RoadSmall,
	RoadMedium,
	RoadLarge
};

class SCR_TW_CompositionHandler: SCR_BaseGameModeComponent
{
	static SCR_TW_CompositionHandler _instance;	
	
	private ref map<string, ref array<SCR_AmbientPatrolSpawnPointComponent>> PatrolSites = new ref map<string, ref array<SCR_AmbientPatrolSpawnPointComponent>>;
	private ref map<SCR_TW_SlotType, ref array<SCR_SiteSlotEntity>> GlobalSites = new ref map<SCR_TW_SlotType, ref array<SCR_SiteSlotEntity>>;
	int small = 0;
	int failed = 0;
	
	void ProcessEntity(SCR_SiteSlotEntity slot)
	{
		string prefabName = slot.GetPrefabData().GetPrefabName();
		string name = slot.GetName();
		if(prefabName.Contains("E_SlotFlatSmall"))
		{
			AddSlot(SCR_TW_SlotType.SiteSmall, slot);
			small++;
		}
		else if(prefabName.Contains("E_SlotFlatMedium"))
			AddSlot(SCR_TW_SlotType.SiteMedium, slot);
		else if(prefabName.Contains("E_SlotFlatLarge"))
			AddSlot(SCR_TW_SlotType.SiteLarge, slot);
		else if(prefabName.Contains("E_SlotRoadSmall"))
			AddSlot(SCR_TW_SlotType.RoadSmall, slot);
		else if(prefabName.Contains("E_SlotRoadMedium"))
			AddSlot(SCR_TW_SlotType.RoadMedium, slot);
		else if(prefabName.Contains("E_SlotRoadLarge"))
			AddSlot(SCR_TW_SlotType.RoadLarge, slot);
		else
		{
			AddSlot(SCR_TW_SlotType.SiteSmall, slot);
			/*Print("No Idea", LogLevel.WARNING);
			
			vector position = slot.GetOrigin();
			
			EntitySpawnParams params();
			vector mat[4];
			mat[3] = position;
			params.TransformMode = ETransformMode.WORLD;
			params.Transform = mat;
			
			Resource resource = Resource.Load(debugIcon);			
			IEntity entity = GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params);*/
		}
			
	}
		
	private void AddSlot(SCR_TW_SlotType type, SCR_SiteSlotEntity slot)
	{
		if(!GlobalSites.Contains(type))
			GlobalSites.Insert(type, new ref array<SCR_SiteSlotEntity>());
		
		GlobalSites.Get(type).Insert(slot);
	}
	
	void OnFactionChange(SCR_CampaignMilitaryBaseComponent base, bool shouldDefendBase=true)
	{
		if(!base)
			return;
		string baseName = base.GetBaseName();
		// This will take spawn points associated to the base, grab their AI groups, and send them to the base location
		if(!PatrolSites.Contains(baseName))
		{
			Print(string.Format("This base does not have any patrol points associated to it: %1", baseName), LogLevel.WARNING);
			return;			
		}
		
		foreach(SCR_AmbientPatrolSpawnPointComponent spawnPoint : PatrolSites.Get(baseName))
		{			
			SCR_AIGroup group = spawnPoint.GetSpawnedGroup();
			
			if(!group)
				continue;
			
			ref array<AIWaypoint> waypoints = {};
			group.GetWaypoints(waypoints);
			
			// Clean out their waypoints
			foreach(AIWaypoint waypoint : waypoints)
				group.RemoveWaypoint(waypoint);
			
			
			
			if(shouldDefendBase)
			{
				AIWaypoint waypoint = SCR_TW_Util.CreateWaypointAt(waypointsConfig.GetAttackWaypointPrefab(), SCR_TW_Util.RandomPositionAround(base.GetOwner(), 75));
				
				if(!waypoint)
					continue;
				group.AddWaypoint(waypoint);
			}			
			else
			{
				vector position = spawnPoint.GetOwner().GetOrigin();
				float staticChance = Math.RandomInt(0, 100);
				
				if(staticChance < 50)
				{
					AIWaypoint waypoint = SCR_TW_Util.CreateWaypointAt(waypointsConfig.GetDefendWaypointPrefab(), position);
					if(!waypoint)
						continue;
					group.AddWaypoint(waypoint);
				}
				else
				{
					SCR_TW_Util.CreatePatrolPathFor(group, waypointsConfig.GetPatrolWaypointPrefab(), waypointsConfig.GetCycleWaypointPrefab(), Math.RandomInt(3, 10), 200);
				}
			}
			
			
		}
	}
	
	
	[Attribute("", UIWidgets.Object, category: "Remnants")]
	private ResourceName remnantSpawnPointPrefab;
	
	[Attribute("", UIWidgets.Object, category: "Remnants")]
	private ResourceName debugIcon;
	
	[Attribute("1", UIWidgets.Slider, params: "0 20 1", category: "Remnants", desc: "Minimum number of remnant spawn points per composition")]
	private int minimumRemnantSpawnPoints;
	
	[Attribute("5", UIWidgets.Slider, params: "1 20 1", category: "Remnants", desc: "Maximum number of remnant spawn points per composition")]
	private int maximumRemnantSpawnPoints;
	
	[Attribute("", UIWidgets.Object, params: "confg class=TW_CompositionConfig", category: "Configs")]
	private ref TW_CompositionConfig bluforConfig;
	
	[Attribute("", UIWidgets.Object, params: "confg class=TW_CompositionConfig", category: "Configs")]
	private ref TW_CompositionConfig opforConfig;
	
	[Attribute("", UIWidgets.Object, params: "confg class=TW_CompositionConfig", category: "Configs")]
	private ref TW_CompositionConfig indforConfig;	
	
	[Attribute("", UIWidgets.Object, params: "confg class=TW_GroupWaypoints", category: "Configs")]
	private ref TW_GroupWaypoints waypointsConfig;
	
	private static SCR_AmbientPatrolManager patrolManager;
	
	SCR_AmbientPatrolManager GetPatrolManager()
	{
		if(patrolManager)
			return patrolManager;
		
		patrolManager = SCR_AmbientPatrolManager.GetInstance(false);
		
		return patrolManager;
	}
	
	protected override void OnPostInit(IEntity owner)
	{
		if(!GetGame().InPlayMode())
			return;
		
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		
		if(rpl && !rpl.IsMaster())
			return;				
		
		GetGame().GetCallqueue().CallLater(QuerySlots, SCR_TW_Util.FromSecondsToMilliseconds(10), false);
	}
	
	private void QuerySlots()
	{
		ref array<SCR_CampaignMilitaryBaseComponent> bases = new ref array<SCR_CampaignMilitaryBaseComponent>;
		SCR_TW_Component.GetInstance().GetCampaignBaseManager().GetCampaignBases(bases);
				
		if(bases.IsEmpty())
		{
			Print("Nothing to query for composition handler. Zero bases found", LogLevel.ERROR);
			return;
		}
				
		// TODO: Figure out how to foreach this without calling it by hand...
		// Was breaking 
		HandleType(SCR_TW_SlotType.SiteSmall);
		HandleType(SCR_TW_SlotType.RoadLarge);
		HandleType(SCR_TW_SlotType.RoadMedium);
		HandleType(SCR_TW_SlotType.RoadSmall);		
		HandleType(SCR_TW_SlotType.SiteMedium);
		HandleType(SCR_TW_SlotType.SiteLarge);		
	}
	
	void HandleType(SCR_TW_SlotType type)
	{
		if(!GlobalSites.Contains(type))
		{
			Print(string.Format("No slots of type: %1", type), LogLevel.ERROR);
			return;
		}
		else
		{
			Print(string.Format("Slots of type %1: %2", type, GlobalSites.Get(type).Count()), LogLevel.WARNING);
		}
		
		foreach(SCR_SiteSlotEntity slot : GlobalSites.Get(type))
		{
			ResourceName prefab = ResourceName.Empty;
			
			switch(type)
			{
				case SCR_TW_SlotType.RoadSmall:
					prefab = indforConfig.GetRandomSmallCheckpointPrefab();		
					break;
				case SCR_TW_SlotType.RoadMedium:
					prefab = indforConfig.GetRandomMediumCheckpointPrefab();
					break;
				case SCR_TW_SlotType.RoadLarge:
					prefab = indforConfig.GetRandomLargeCheckpointPrefab();
					break;
				case SCR_TW_SlotType.SiteSmall:
					prefab = indforConfig.GetRandomSmallSitePrefab();
					break;
				case SCR_TW_SlotType.SiteMedium:
					prefab = indforConfig.GetRandomMediumSitePrefab();
					break;
				case SCR_TW_SlotType.SiteLarge:
					prefab = indforConfig.GetRandomLargeSitePrefab();
					break;
				default:
					prefab = indforConfig.GetRandomSmallSitePrefab();
					break;			
			}
			
			SpawnSite(prefab, slot);
			
			if(!spawnedCount.Contains(prefab))
			{
				if(!failedCount.Contains(prefab))
					failedCount.Insert(prefab, 0);
				failedCount.Set(prefab, failedCount.Get(prefab) + 1);
			}
		}
	}
	
	ref map<ResourceName, int> spawnedCount = new map<ResourceName, int>;
	ref map<ResourceName, int> failedCount = new map<ResourceName, int>;
	
	void SpawnSite(ResourceName resourceName, SCR_SiteSlotEntity slot)
	{
		if(slot == null)
		{
			Print("Slot is null, this won't work");
			return;
		}
		
		if(resourceName.IsEmpty())
		{
			Print("Invalid resource name");
			return;
		}
		
		// Bad things happen if you try spawning a slot that is occupied
		if(slot.IsOccupied()) return;
		
		SCR_CampaignMilitaryBaseComponent base = SCR_TW_Component.GetGlobalCampaign().GetBaseManager().FindClosestBase(slot.GetOrigin());
		
		// Avoid spawning stuff around an HQ since this is where player spawn
		if(base.IsHQ())
		{
			Print(string.Format("Too close to %1 to spawn", base.GetBaseName()), LogLevel.WARNING);
			float distance = vector.Distance(slot.GetOrigin(), base.GetOwner().GetOrigin());
			if(distance < 200)
				return;
		}				
		
		/*
			Each site slot has a rotation applied to it. Appears to be how the world creator wanted things to line up
			So we shall take this into consideration and apply the Y rotation to each composition we spawn
		
			So far this seems to work as intended
		*/
		vector angles = slot.GetAngles();
		Resource resource = Resource.Load(resourceName);
		
		IEntity composition = slot.SpawnEntityInSlot(resource, angles[1]); // Y Angle is #2, which equates to index 1
		
		if(composition)
		{
			if(!spawnedCount.Contains(resourceName))
				spawnedCount.Insert(resourceName, 0);
			spawnedCount.Set(resourceName, spawnedCount.Get(resourceName) + 1);
		}	
		
		if(!GetPatrolManager())
		{
			Print("Patrol Manager was not found. Unable to register patrols");
			return;
		}
		
		if(remnantSpawnPointPrefab)
		{
			int spawnCount = Math.RandomIntInclusive(minimumRemnantSpawnPoints, maximumRemnantSpawnPoints);
			
			Resource patrolResource = Resource.Load(remnantSpawnPointPrefab);
			for(int i = 0; i < spawnCount; i++)
			{				
				EntitySpawnParams params();
				
				vector mat[4];
				vector position = SCR_TW_Util.RandomPositionAround(slot, 100, 0);
				
				mat[3] = position;
				params.TransformMode = ETransformMode.WORLD;
				params.Transform = mat;
				
				IEntity entity = GetGame().SpawnEntityPrefab(patrolResource, GetGame().GetWorld(), params);
				SCR_AmbientPatrolSpawnPointComponent ap = SCR_AmbientPatrolSpawnPointComponent.Cast(entity.FindComponent(SCR_AmbientPatrolSpawnPointComponent));
				
				if(!PatrolSites.Contains(base.GetBaseName()))
				{
					PatrolSites.Insert(base.GetBaseName(), new ref array<SCR_AmbientPatrolSpawnPointComponent>());
					PatrolSites.Get(base.GetBaseName()).Insert(ap);
				}
				else
					PatrolSites.Get(base.GetBaseName()).Insert(ap);
			}
		}		
		
		if(composition.GetChildren())
		{
			SCR_AmbientPatrolSpawnPointComponent patrolPoint = GetPatrolSpawnPoint(composition);
			
			if(patrolPoint)		
				GetPatrolManager().RegisterPatrol(patrolPoint);
		}
	}
	
	SCR_AmbientPatrolSpawnPointComponent GetPatrolSpawnPoint(IEntity root)
	{
		SCR_AmbientPatrolSpawnPointComponent comp = SCR_AmbientPatrolSpawnPointComponent.Cast(root.FindComponent(SCR_AmbientPatrolSpawnPointComponent));
		
		if(comp) return comp;
		
		IEntity current;
		
		if(root.GetChildren())
		{
			current = root.GetChildren();
			while(current)
			{
				comp = SCR_AmbientPatrolSpawnPointComponent.Cast(current.FindComponent(SCR_AmbientPatrolSpawnPointComponent));
				if(comp) return comp;
				current = current.GetSibling();
			}
		}
		
		return null;
	}
	
	static SCR_TW_CompositionHandler GetInstance()
	{
		if(_instance)
			return _instance;
		
		_instance = SCR_TW_CompositionHandler.Cast(GetGame().GetGameMode().FindComponent(SCR_TW_CompositionHandler));
		return _instance;
	}
};