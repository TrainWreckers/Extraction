class SCR_TW_Util 
{	
	private static ref map<FactionKey, ref array<ResourceName>> _factionGroups = new map<FactionKey, ref array<ResourceName>>();
	
	//! Output x and y coordinates for center of grid square
	static void GetCenterOfGridSquare(vector position, out int x, out int y, int gridSize = 1000)
	{
		x = (int)(position[0] / gridSize);
		y = (int)(position[2] / gridSize);
		
		x = (x * gridSize) + (gridSize / 2);
		y = (y * gridSize) + (gridSize / 2);
	}
	
	static void AddSurroundingGridSquares(notnull inout set<string> chunks, vector position, int radius = 1, int gridSize = 1000)
	{
		int x = (int)(position[0] / gridSize);
		int y = (int)(position[2] / gridSize);
		
		int xStart = x - radius;
		int xEnd = x + radius;
		int yStart = y - radius;
		int yEnd = y + radius;
		
		string text = "";
		string combined = "";
		for(int gx = xStart; gx <= xEnd; gx++)
		{
			for(int gy = yStart; gy <= yEnd; gy++)
			{
				text = string.Format("%1 %2", gx, gy);
				combined += string.Format("(%1) ", text);
				if(!chunks.Contains(text))
					chunks.Insert(text);
			}
		}
		
		Print(string.Format("TrainWreck: Grid Squares: %1", combined));
	}
	
	static string ToGridText(vector position, int gridSize = 1000)
	{
		int x = (int)(position[0] / gridSize);
		int y = (int)(position[2] / gridSize);
		return string.Format("%1 %2", x, y);
	}
	
	static void ToGrid(vector position, out int x, out int y)
	{
		x = (int)(position[0] / 1000);
		y = (int)(position[2] / 1000);
	}
	
	static void FromGridString(string grid, out int x, out int y)
	{
		ref array<int> nums = SCR_StringHelper.GetIntsFromString(grid);
		
		if(nums.Count() >= 2)
		{
			x = nums[0];
			y = nums[1];
		}
	}
	
	static bool IsValidWanderer_Agent(SCR_ChimeraAIAgent agent)
	{
		if(!agent) return false;
		
		AIGroup aiGroup = agent.GetParentGroup();
		
		if(!aiGroup) return false;
		
		SCR_AIGroup group = SCR_AIGroup.Cast(aiGroup);
		
		if(group && group.IgnoreWanderingSystem())
			return false;
		
		return true;
	}
	
	private void InitializeFactionGroups()
	{
		ref array<int> playerIds = {};
		int playerCount = GetGame().GetPlayerManager().GetPlayers(playerIds);
		
		if(playerCount <= 0)
		{
			Print("TrainWreck: No players are available to grab enemy faction(s) for", LogLevel.ERROR);
			return;
		}
		
		int playerId = playerIds.Get(0);
		
		SCR_FactionManager manager = SCR_FactionManager.Cast(GetGame().GetFactionManager());
		Faction playerFaction = manager.GetPlayerFaction(playerId);		
		FactionKey playerFactionKey = playerFaction.GetFactionKey();
		
		if(!manager)
			Debug.Error("TrainWreck: Extraction requires a faction manager to be present");
		
		ref array<Faction> factions = {};
		int count = manager.GetFactionsList(factions);
		
		for(int i = 0; i < count; i++)
		{			
			SCR_Faction faction = SCR_Faction.Cast(factions.Get(i));
			if(faction.GetFactionKey() == playerFactionKey)
				continue;
						
			ref array<SCR_EntityCatalog> catalogs = {};
			int catalogCount = faction.GetAllFactionEntityCatalogs(catalogs);
			
			if(catalogCount <= 0)
			{
				Print(string.Format("TrainWreck: %1 does not have catalogs", faction.GetFactionKey()), LogLevel.WARNING);
				continue;
			}
			
			ref array<ResourceName> factionPrefabs = {};
			
			foreach(SCR_EntityCatalog catalog : catalogs)
			{
				if(catalog.GetCatalogType() != EEntityCatalogType.GROUP)
					continue;
				
				ref array<SCR_EntityCatalogEntry> entries = {};
				catalog.GetEntityList(entries);
				
				foreach(SCR_EntityCatalogEntry entry : entries)
					factionPrefabs.Insert(entry.GetPrefab());
			}
			
			_factionGroups.Insert(faction.GetFactionKey(), factionPrefabs);
		}				
	}
	
	//! Get any faction not the player --> grab the groups for it
	void GetEnemyFactionGroups(out notnull array<ResourceName> prefabs)
	{
		if(_factionGroups.IsEmpty())
			InitializeFactionGroups();
		
		if(_factionGroups.IsEmpty())
			return;
		
		int count = _factionGroups.Count();
		int index = Math.RandomInt(0, count);
		
		FactionKey randomKey = _factionGroups.GetKey(index);
		prefabs.Copy(_factionGroups.Get(randomKey));
	}
	
	IEntity GetProviderFromRplId(RplId rplProviderId)
	{
		RplComponent rplComp = RplComponent.Cast(Replication.FindItem(rplProviderId));

		if (!rplComp)
			return null;

		return rplComp.GetEntity();
	}
	
	static ref RandomGenerator random = new RandomGenerator();
	
	static BaseWorld GetWorld()
	{
		ArmaReforgerScripted game = GetGame();
		BaseWorld world = game.GetWorld();
		return world;
	}
	
	static vector GetForwardVec(IEntity entity)
	{
		return entity.GetTransformAxis(0).Normalized();
	}
	
	// Convert seconds to milliseconds
	static int FromSecondsToMilliseconds(float seconds)
	{
		return seconds * 1000;
	}
	
	// Convert minutes to milliseconds
	static int FromMinutesToMilliseconds(int minutes)
	{
		return FromSecondsToMilliseconds(minutes * 60);
	}
	
	static void GetFactionBaseDict(FactionKey playerFaction, array<SCR_CampaignMilitaryBaseComponent> allBases, out notnull map<FactionKey, ref array<SCR_CampaignMilitaryBaseComponent>> factionMap)
	{
		foreach(SCR_CampaignMilitaryBaseComponent base : allBases)
		{
			FactionKey baseFaction = base.GetFaction(true).GetFactionKey();
			
			if(!baseFaction)
			{
				Print(string.Format("Was unable to retrieve faction %1 for base %2", baseFaction, base.GetBaseName()), LogLevel.WARNING);				
				
				if(factionMap.Contains("default"))
					factionMap.Get("default").Insert(base);
				else
				{
					factionMap.Insert("default", new ref array<SCR_CampaignMilitaryBaseComponent>);
					factionMap.Get("default").Insert(base);
				}
				continue;
			}				
			
			if(factionMap.Contains(baseFaction))
				factionMap.Get(baseFaction).Insert(base);
			else
			{
				factionMap.Insert(baseFaction, new ref array<SCR_CampaignMilitaryBaseComponent>);
				factionMap.Get(baseFaction).Insert(base);
			}
		}				
	}
	
	/*
		Returns true if worldPosition is water
	*/
	static bool IsWater(vector worldPosition)
	{
		bool isWater = false;
		BaseWorld world = GetWorld();
		
		float surfaceHeight = world.GetSurfaceY(worldPosition[0], worldPosition[2]);
		
		worldPosition[1] = surfaceHeight;
		
		float lakeArea;
		EWaterSurfaceType waterSurfaceType;
		
		float waterSurfaceY = GetWaterSurfaceY(world, worldPosition, waterSurfaceType, lakeArea);
		if(waterSurfaceType == EWaterSurfaceType.WST_OCEAN || waterSurfaceType == EWaterSurfaceType.WST_POND)
			isWater = true;
		
		return isWater;
	}
	
	/*Obtain the Y coordinate at a given position. Also output the surface type*/
	static float GetWaterSurfaceY(BaseWorld world, vector worldPos, out EWaterSurfaceType waterSurfaceType, out float lakeArea)
	{
		vector obbExtens;
		vector transformWS[4];
		vector waterSurfacePos;
		
		ChimeraWorldUtils.TryGetWaterSurface(world, worldPos, waterSurfacePos, waterSurfaceType, transformWS, obbExtens);
		
		lakeArea = obbExtens[0] * obbExtens[2];
		
		return waterSurfacePos[1];
	}
	
	/*Provide a random position around a given point.*/
	static vector RandomPositionAround(IEntity point, int radius, int minimumDistance = 0)
	{
		vector position = random.GenerateRandomPointInRadius(minimumDistance, radius, point.GetOrigin());
		
		while(IsWater(position))
		{
			Print("RandomPointAround: Attempting to find a land-based position...", LogLevel.DEBUG);
			position = RandomPositionAround(point, Math.Max(radius * 0.9, minimumDistance), minimumDistance);
		}
		
		return position;
	}
	
	/*Provide a random position that's also a certain distance away from players*/
	static vector RandomPositionAroundButAlso(IEntity point, notnull array<IEntity> players, int radius, int minimumDistance = 0)
	{
		while(true)
		{
			vector position = RandomPositionAround(point, radius, minimumDistance);
			bool skip = false;
			// We cannot be within minimum spawn distance of a player
			foreach(IEntity player : players)
			{
				if(vector.Distance(player.GetOrigin(), position) < minimumDistance)
				{
					skip = true;
					break;
				}				
			}
			
			if(skip)
				continue;
			
			return position;
		}
		
		return vector.One;
	}
	
	/*Provide a random position around a given point*/
	static vector RandomPositionAroundPoint(vector position, int radius, int minimumDistance = 0)
	{				
		// TODO: Why does this method not work 
		// IS it recursion? ... this works when called directly.
		if(radius < minimumDistance)
			radius = minimumDistance;
				
		vector center = random.GenerateRandomPointInRadius(minimumDistance, radius, position);
		
		while(IsWater(center))
		{
			float reducedRadius = radius * 0.95;
			float newRadius = Math.Max(reducedRadius, minimumDistance);
			if(newRadius < minimumDistance)
				newRadius = minimumDistance;
			Print("RandomPositionAroundPoint: Attempting to find a land-based position...", LogLevel.DEBUG);
			center = RandomPositionAroundPoint(position, newRadius, minimumDistance);
		}
		
		Print(string.Format("Radius: %1 | Minimum: %2 | Pos: %3", radius, minimumDistance, center));
		return position;
	}
	
	/*Spawn a group of AI at a given point*/
	static SCR_AIGroup SpawnGroup(ResourceName groupPrefab, vector center, int radius, int minimumDistance = 0)
	{
		if(!groupPrefab || groupPrefab.IsEmpty()) 
			return null;
		
		if(radius < minimumDistance)
			radius = minimumDistance;
		
		Resource resource = Resource.Load(groupPrefab);
		
		if(!resource || !resource.IsValid())
		{
			Print(string.Format("TrainWreck: Invalid Group Prefab: %1", groupPrefab), LogLevel.ERROR);
			return null;
		}
		
		EntitySpawnParams params = EntitySpawnParams();
		params.TransformMode = ETransformMode.WORLD;				
		
		if(radius <= 4)
			params.Transform[3] = center;
		else
			params.Transform[3] = random.GenerateRandomPointInRadius(minimumDistance, radius, center); //RandomPositionAroundPoint(center, radius, minimumDistance);
		
		int attempts = 0;
		while(IsWater(params.Transform[3]) && attempts < 30)
		{
			params.Transform[3] = random.GenerateRandomPointInRadius(minimumDistance, Math.ClampInt(radius-(attempts*25), minimumDistance, int.MAX), center);
			attempts++;
		}
		
		return SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params));
	}
	
	
	/*Get a position between two points*/
	static vector PositionBetween(vector origin, vector destination, int distance)
	{
		float angle = Math.Atan2(destination[2] - origin[2], destination[0] - origin[0]);
		
		vector pos = vector.Zero;
		
		pos[0] = origin[0] - (distance * Math.Sin(angle));
		pos[2] = origin[2] - (distance * Math.Cos(angle));
		pos[1] = GetGame().GetWorld().GetSurfaceY(pos[0], pos[2]);
		
		return pos;
	}	
	
	/*Generate a patrol path for a given group*/
	static void CreatePatrolPathFor(SCR_AIGroup group, ResourceName waypointPrefab, ResourceName cyclePrefab, int waypointCount, float radius)
	{
		vector currentPoint = GetLandPositionAround(group.GetOrigin(), radius);
		
		for(int i = 0; i < waypointCount; i++)
		{
			AIWaypoint waypoint = CreateWaypointAt(waypointPrefab, currentPoint);
			
			if(!waypoint)
				continue;
			
			group.AddWaypoint(waypoint);
			currentPoint = GetLandPositionAround(currentPoint, radius);
		}
		
		AIWaypoint cycle = CreateWaypointAt(cyclePrefab, group.GetOrigin());
		if(!cycle)
			return;
		
		group.AddWaypoint(cycle);			
	}
	
	/*Spawn a group around a given location*/
	static SCR_AIGroup SpawnGroup(ResourceName groupPrefab, IEntity locationEntity, int radius, int minimumDistance = 0)
	{
		if(!groupPrefab || !locationEntity) return null;
		return SpawnGroup(groupPrefab, locationEntity.GetOrigin(), radius, minimumDistance);
	}
	
	/*Enforce obtaining a land position around a given point*/
	static vector GetLandPositionAround(vector center, int radius)
	{
		vector mat[4]; // IDK if this is actually needed
		vector position = mat[3];
		int attempts = 25;
		
		position = random.GenerateRandomPointInRadius(0, radius, center);
		position[1] = GetWorld().GetSurfaceY(position[0], position[2]);
		
		/*
			Algorithm - currently, the center point is on land, because it's a campaign base.
			So if we reduce our radius, we should eventually find land 
			
			Maximum algorithm
			100 radius 
				25/25 = 1 
				100 * 1 = 100 
		
			Shrunken algorithm 
			100 radius 
				20/25 = 0.80
				100 * 0.80 = 80 
		*/
		
		int currentRadius = radius * (attempts/25);
					
		// Yes, there is still technically a chance units may spawn in the water
		// Hoping that this algorithm reduces that chance 
		while(IsWater(position) && attempts > 0)
		{
			attempts -= 1;
			position = random.GenerateRandomPointInRadius(0, currentRadius, center);
			position[1] = GetWorld().GetSurfaceY(position[0], position[2]);
		}
		
		return position;
	}
	
	static vector GetLandPositionAround(IEntity center, int radius)
	{
		return GetLandPositionAround(center.GetOrigin(), radius);	
	}
	
	static bool IsOutsideOfPlayers(vector position, notnull array<IEntity> entities, float distance)
	{
		foreach(IEntity entity : entities)
		{
			if(!entity)
				continue;
			
			if(vector.Distance(entity.GetOrigin(), position) < distance)
				return false;
		}
		
		return true;
	}
	
	static bool IsWithinRange(vector position, array<IEntity> entities, float min, float max)
	{
		foreach(IEntity entity : entities)
		{
			// Invalid entity to work with 
			if(!entity) 
				continue;
			
			float distance = Math.AbsFloat(vector.Distance(entity.GetOrigin(), position));
			
			// Is this point at least close to 1 player?
			if(distance >= min && distance <= max)
				return true;		
		}
		
		return false;
	}
	
	static string ArsenalTypeAsString(SCR_EArsenalItemType type)
	{
		switch(type)
		{
			case SCR_EArsenalItemType.HEAL: return "HEAL";
			case SCR_EArsenalItemType.LEGS: return "LEGS";
			case SCR_EArsenalItemType.TORSO: return "TORSO";
			case SCR_EArsenalItemType.RIFLE: return "RIFLE";
			case SCR_EArsenalItemType.PISTOL: return "PISTOL";
			case SCR_EArsenalItemType.FOOTWEAR: return "FOOTWEAR";
			case SCR_EArsenalItemType.BACKPACK: return "BACKPACK";
			case SCR_EArsenalItemType.HEADWEAR: return "HEADWEAR";
			case SCR_EArsenalItemType.EQUIPMENT: return "EQUIPMENT";
			case SCR_EArsenalItemType.EXPLOSIVES: return "EXPLOSIVES";
			case SCR_EArsenalItemType.MACHINE_GUN: return "MACHINE_GUN";
			case SCR_EArsenalItemType.SNIPER_RIFLE: return "SNIPER_RIFLE";
			case SCR_EArsenalItemType.RADIO_BACKPACK: return "RADIO_BACKPACK";
			case SCR_EArsenalItemType.VEST_AND_WAIST: return "VEST_AND_WAIST";
			case SCR_EArsenalItemType.ROCKET_LAUNCHER: return "ROCKET_LAUNCHER";
			case SCR_EArsenalItemType.LETHAL_THROWABLE: return "LETHAL_THROWABLE";
			case SCR_EArsenalItemType.WEAPON_ATTACHMENT: return "WEAPON_ATTACHMENT";
			case SCR_EArsenalItemType.NON_LETHAL_THROWABLE: return "NON_LETHAL_THROWABLE";			
		}
		
		return "HEAL";
	}
	
	/*Delete group and its members*/
	static void DeleteGroup(SCR_AIGroup group)
	{
		if(!group) return;
		
		array<SCR_ChimeraCharacter> members = group.GetAIMembers();
		foreach(SCR_ChimeraCharacter member : members)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(member);
		}
		
		SCR_EntityHelper.DeleteEntityAndChildren(group);
	}
	
	/*Create a waypoint at a given position*/
	static AIWaypoint CreateWaypointAt(ResourceName waypointPrefab, vector waypointPosition)
	{
		Resource resource = Resource.Load(waypointPrefab);
		
		if(!resource) return null;
		
		AIWaypoint wp = AIWaypoint.Cast(GetGame().SpawnEntityPrefab(resource));
		if(!wp) return null;
		
		wp.SetOrigin(waypointPosition);
		return wp;
	}	
	
	static bool IsMagazineNotFull(MagazineComponent magazine)
	{
		return magazine.GetAmmoCount() < magazine.GetMaxAmmoCount();
	}
	
	static bool CharacterMagCount(MagazineComponent currentMag, SCR_InventoryStorageManagerComponent storageManager)
	{
		SCR_MagazinePredicate magPredicate = new SCR_MagazinePredicate();
		BaseMagazineWell magWell = currentMag.GetMagazineWell();
		
		magPredicate.magWellType = magWell.Type();
		
		array<IEntity> magEntities = new array<IEntity>();
		int magCount = storageManager.FindItems(magEntities, magPredicate);
		return magCount > 1;
	}
	
	static array<MagazineComponent> SortedMagazines(notnull array<IEntity> magEntities)
	{
		array<MagazineComponent> mags = new array<MagazineComponent>();		
		array<int> tempArray = new array<int>();
		
		foreach(IEntity item : magEntities)
		{
			MagazineComponent mag = MagazineComponent.Cast(item.FindComponent(MagazineComponent));
			int ammoCount = mag.GetAmmoCount();
			
			if(tempArray.Contains(ammoCount))
				continue;
			
			tempArray.Insert(mag.GetAmmoCount());
		}
		
		tempArray.Sort();
		
		foreach(int count : tempArray)
		{
			foreach(IEntity item : magEntities)
			{
				MagazineComponent mag = MagazineComponent.Cast(item.FindComponent(MagazineComponent));
				if(mag.GetAmmoCount() == count)
					mags.Insert(mag);
			}
		}
		
		return mags;
	}
	
	static void ResetInventoryMenu()
	{
		MenuManager menuManager = GetGame().GetMenuManager();
		menuManager.CloseAllMenus();
		SCR_InventoryMenuUI inventoryMenu = SCR_InventoryMenuUI.Cast(menuManager.OpenMenu(ChimeraMenuPreset.Inventory20Menu));
	}
	
	protected static ref map<ResourceName, ref UIInfo> s_ItemUIInfo = new map<ResourceName, ref UIInfo>();
	protected static ref map<ResourceName, ref SCR_EditableVehicleUIInfo> s_VehicleUIInfo = new map<ResourceName, ref SCR_EditableVehicleUIInfo>();
	
	static UIInfo GetItemUIInfo(ResourceName prefab)
	{
		UIInfo resultInfo = s_ItemUIInfo.Get(prefab);
		
		if(!resultInfo)
		{
			IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(Resource.Load(prefab));
			
			if(entitySource)
			{
				for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
				{
					IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
					
					if(componentSource.GetClassName().ToType().IsInherited(InventoryItemComponent))
					{
						BaseContainer attributesContainer = componentSource.GetObject("Attributes");
						if(attributesContainer)
						{
							BaseContainer itemDisplayNameContainer = attributesContainer.GetObject("ItemDisplayName");
							if(itemDisplayNameContainer)
							{
								resultInfo = UIInfo.Cast(BaseContainerTools.CreateInstanceFromContainer(itemDisplayNameContainer));
								break;
							}
						}
					}
				}
			}
			
			s_ItemUIInfo.Set(prefab, resultInfo);
		}
		
		return resultInfo;
	}
	
	static SCR_EditableVehicleUIInfo GetVehicleUIInfo(ResourceName prefab)
	{
		SCR_EditableVehicleUIInfo resultInfo = s_VehicleUIInfo.Get(prefab);
		
		if(!resultInfo)
		{
			IEntitySource entitySource = SCR_BaseContainerTools.FindEntitySource(Resource.Load(prefab));
			
			if(entitySource)
			{
				for(int nComponent, componentCount = entitySource.GetComponentCount(); nComponent < componentCount; nComponent++)
				{
					IEntityComponentSource componentSource = entitySource.GetComponent(nComponent);
					
					if(componentSource.GetClassName().ToType().IsInherited(SCR_EditableVehicleComponent))
					{
						BaseContainer baseUIInfo = componentSource.GetObject("m_UIInfo");
						if(baseUIInfo)
						{
							resultInfo = SCR_EditableVehicleUIInfo.Cast(BaseContainerTools.CreateInstanceFromContainer(baseUIInfo));
							break;
						}
					}
				}
			}
			
			s_VehicleUIInfo.Set(prefab, resultInfo);
		}
		
		return resultInfo;
	}
	
	static string GetPrefabDisplayName(ResourceName prefab)
	{
		SCR_EditableVehicleUIInfo uiInfo = GetVehicleUIInfo(prefab);
		
		if(uiInfo)
			return uiInfo.GetName();
		
		UIInfo itemUIInfo = GetItemUIInfo(prefab);
		if(itemUIInfo)
			return itemUIInfo.GetName();
		
		return prefab;
	}
	
	static string GetPrefabDescription(ResourceName prefab)
	{
		SCR_EditableVehicleUIInfo uiInfo = GetVehicleUIInfo(prefab);
		
		if(uiInfo)
			return uiInfo.GetDescription();
		
		UIInfo itemInfo = GetItemUIInfo(prefab);
		
		if(itemInfo)
			return itemInfo.GetDescription();
				
		return prefab;
	}
	
	static ResourceName GetPrefabDisplayIcon(ResourceName prefab)
	{
		SCR_EditableVehicleUIInfo uiInfo = GetVehicleUIInfo(prefab);
		
		if(uiInfo)
			return uiInfo.GetIconPath();
		
		UIInfo itemInfo = GetItemUIInfo(prefab);
		if(itemInfo)
			return itemInfo.GetIconPath();
		
		return string.Empty;
	}
	
	static bool InsertAutoEquipItem(SCR_InventoryStorageManagerComponent inventory, IEntity item)
	{
		EStoragePurpose purpose = EStoragePurpose.PURPOSE_ANY;
		if(item.FindComponent(WeaponComponent)) purpose = EStoragePurpose.PURPOSE_WEAPON_PROXY;
		if(item.FindComponent(BaseLoadoutClothComponent)) purpose = EStoragePurpose.PURPOSE_LOADOUT_PROXY;
		if(item.FindComponent(SCR_GadgetComponent)) purpose = EStoragePurpose.PURPOSE_GADGET_PROXY;
		
		bool insertedItem = inventory.TryInsertItem(item, purpose, null);
		
		if(!insertedItem)
			insertedItem = inventory.TryInsertItem(item, EStoragePurpose.PURPOSE_ANY, null);
		
		return insertedItem;
	}
};