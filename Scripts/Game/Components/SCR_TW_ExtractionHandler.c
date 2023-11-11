[EntityEditorProps(category: "GameScripted/TrainWreck/Systems", description: "Handles extraction related setup at a game-mode level")]
class SCR_TW_ExtractionHandlerClass : SCR_BaseGameModeComponentClass {};

class SCR_TW_ExtractionHandler : SCR_BaseGameModeComponent
{
	static SCR_TW_ExtractionHandler s_Instance;
	
	static SCR_TW_ExtractionHandler GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		
		if(!gameMode)
			return null;
		
		if(!s_Instance)
			s_Instance = SCR_TW_ExtractionHandler.Cast(gameMode.FindComponent(SCR_TW_ExtractionHandler));
		
		return s_Instance;
	}
	
	private ref map<SCR_EArsenalItemType, ref array<SCR_ArsenalItem>> lootMap = new map<SCR_EArsenalItemType, ref array<SCR_ArsenalItem>>();
	private ref map<SCR_EArsenalItemType, ref array<ref TW_LootConfigItem>> lootTable = new ref map<SCR_EArsenalItemType, ref array<ref TW_LootConfigItem>>();	
	private ref map<int, SCR_TW_PlayerCrateComponent> crates = new map<int, SCR_TW_PlayerCrateComponent>();
	private ref array<SCR_SiteSlotEntity> possibleSpawnAreas = {};
	private ref array<SCR_TW_ExtractionSiteComponent> possibleExtractionSites = {};
	
	[Attribute("{CB7CDB3864826FD3}Prefabs/Props/Military/AmmoBoxes/EquipmentBoxStack/TW_PlayerLoadoutCrate.et", UIWidgets.ResourcePickerThumbnail, category: "Player Spawn", desc: "Player loadout crate prefab", params: "et")]
	private ResourceName playerCratePrefab;
		
	[Attribute("{5A52168A894DDB7E}Prefabs/Compositions/Slotted/SlotFlatSmall/TW_US_PlayerHub_Extraction.et", UIWidgets.ResourcePickerThumbnail, params: "et", category: "Player Spawn", desc: "Composition to spawn as a player starting area")]
	private ResourceName playerHubPrefab;
	
	[Attribute("", UIWidgets.Slider, params: "3, 20, 1", category: "Player Spawn", desc: "After this timer elapses, the player spawn composition is deleted")]
	private int playerHubDespawnTimerInMinutes;
	
	[Attribute("1", UIWidgets.Slider, params: "1, 5, 1", category: "Extraction", desc: "Number of potential extractions available at once")]
	private int numberOfExtractionSites;
	
	void RegisterExtractionSite(SCR_TW_ExtractionSiteComponent site)
	{
		if(!possibleExtractionSites.Contains(site))
			possibleExtractionSites.Insert(site);				
	}
	
	void RegisterPlayerLoadoutCrate(SCR_TW_PlayerCrateComponent crate)
	{
		int playerId = crate.GetPlayerId();
		
		if(playerId < 0)
			return;
		
		if(!crates.Contains(playerId))
			crates.Insert(playerId, crate);
		else
			crates.Set(playerId, crate);
	}
	
	private void DeleteCrateLater(IEntity owner)
	{
		SCR_EntityHelper.DeleteEntityAndChildren(owner);
	}
	
	void RegisterSpawnArea(SCR_SiteSlotEntity spawnSlot)
	{
		if(possibleSpawnAreas.Contains(spawnSlot))
			return;
		
		possibleSpawnAreas.Insert(spawnSlot);
	}
	
	override void OnPlayerSpawned(int playerId, IEntity controlledEntity)
	{
		super.OnPlayerSpawned(playerId, controlledEntity);
		
		if(!crates.Contains(playerId))
		{
			Print(string.Format("TrainWreck: No crate registered for player Id: %1", playerId), LogLevel.ERROR);
			return;
		}
		
		SCR_TW_PlayerCrateComponent crate = crates.Get(playerId);
		
		if(!crate)
			return;
		
		crate.InitializeForPlayer(playerId);
		GetGame().GetCallqueue().CallLater(DestoryCrate, 5 * 60 * 1000, false, crate.GetOwner());
	}
	
	private void DestoryCrate(IEntity crate)
	{
		Print("TrainWreck: Deleting Player Crate", LogLevel.WARNING);
		SCR_EntityHelper.DeleteEntityAndChildren(crate);
	}
	
	//------------------------------------------------------------------------------------------------
	//! RPC Call to server to ensure only the server udpates/saves inventory 
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcUpdatePlayerCrate(int playerId)
	{
		if(!crates.Contains(playerId))
			return;
		
		crates.Get(playerId).InitializeForPlayer(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	
	void UpdatePlayerInventoryCrate(int playerId)
	{
		Rpc(RpcUpdatePlayerCrate, playerId);
	}
	
	private bool OutputLootTableFile()
	{
		SCR_JsonSaveContext saveContext = new SCR_JsonSaveContext();
		
		foreach(SCR_EArsenalItemType type, ref array<SCR_ArsenalItem> items : lootMap)
		{
			ref array<ref TW_LootConfigItem> typeLoot = {};
			
			foreach(SCR_ArsenalItem item : items)
			{
				ref TW_LootConfigItem lootItem = new TW_LootConfigItem();
				ResourceName resourceName = item.GetItemResourceName();
				float chance = item.GetItemChanceToSpawn();
				int count = item.GetItemMaxSpawnCount();
				lootItem.SetData(resourceName, chance, count);
				
				typeLoot.Insert(lootItem);
			}
			
			saveContext.WriteValue(SCR_TW_Util.ArsenalTypeAsString(type), typeLoot);
		}
		
		bool success = saveContext.SaveToFile("$profile:lootmap.json");
		
		return success;
	}
	
	private bool HasLootTable()
	{
		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		return loadContext.LoadFromFile("$profile:lootmap.json");
	}
	
	private bool IngestLootTableFromFile()
	{
		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		bool loadSuccess = loadContext.LoadFromFile("$profile:lootmap.json");
		
		if(!loadSuccess)
		{
			Print("TrainWreck: Was unable to load loot map. Please verify it exists, and has valid syntax");
			return false;
		}
				
		if(!LoadSection(loadContext, SCR_EArsenalItemType.HEAL))
			Print("TrainWreck: LootMap: unable to load HEAL", LogLevel.ERROR);
		
		if(!LoadSection(loadContext, SCR_EArsenalItemType.LEGS))
			Print("TrainWreck: LootMap: unable to load LEGS", LogLevel.ERROR);
		
		if(!LoadSection(loadContext, SCR_EArsenalItemType.TORSO))
			Print("TrainWreck: LootMap: unable to load TORSO", LogLevel.ERROR);
		
		if(!LoadSection(loadContext, SCR_EArsenalItemType.RIFLE))
			Print("TrainWreck: LootMap: unable to load RIFLE", LogLevel.ERROR);
		
		if(!LoadSection(loadContext, SCR_EArsenalItemType.PISTOL))
			Print("TrainWreck: LootMap: unable to load PISTOL", LogLevel.ERROR);
		
		if(!LoadSection(loadContext, SCR_EArsenalItemType.FOOTWEAR))
			Print("TrainWreck: LootMap: unable to load FOOTWEAR", LogLevel.ERROR);
		
		if(!LoadSection(loadContext, SCR_EArsenalItemType.BACKPACK))
			Print("TrainWreck: LootMap: unable to load BACKPACK", LogLevel.ERROR);
		
		if(!LoadSection(loadContext, SCR_EArsenalItemType.HEADWEAR))
			Print("TrainWreck: LootMap: unable to load HEADWEAR", LogLevel.ERROR);
		
		if(!LoadSection(loadContext, SCR_EArsenalItemType.EQUIPMENT))
			Print("TrainWreck: LootMap: unable to load EQUIPMENT", LogLevel.ERROR);
		
		if(!LoadSection(loadContext, SCR_EArsenalItemType.EXPLOSIVES))
			Print("TrainWreck: LootMap: unable to load EXPLOSIVES", LogLevel.ERROR);
		
		if(!LoadSection(loadContext, SCR_EArsenalItemType.MACHINE_GUN))
			Print("TrainWreck: LootMap: unable to load MACHINE_GUN", LogLevel.ERROR);
		
		if(!LoadSection(loadContext, SCR_EArsenalItemType.SNIPER_RIFLE))
			Print("TrainWreck: LootMap: unable to load SNIPER_RIFLE", LogLevel.ERROR);		
		
		if(!LoadSection(loadContext, SCR_EArsenalItemType.RADIO_BACKPACK))
			Print("TrainWreck: LootMap: unable to load RADIO_BACKPACK", LogLevel.ERROR);
		
		if(!LoadSection(loadContext, SCR_EArsenalItemType.VEST_AND_WAIST))
			Print("TrainWreck: LootMap: unable to load VEST_AND_WAIST", LogLevel.ERROR);
		
		if(!LoadSection(loadContext, SCR_EArsenalItemType.ROCKET_LAUNCHER))
			Print("TrainWreck: LootMap: unable to load ROCKET_LAUNCHER", LogLevel.ERROR);
		
		if(!LoadSection(loadContext, SCR_EArsenalItemType.LETHAL_THROWABLE))
			Print("TrainWreck: LootMap: unable to load LETHAL_THROWABLE", LogLevel.ERROR);
		
		if(!LoadSection(loadContext, SCR_EArsenalItemType.WEAPON_ATTACHMENT))
			Print("TrainWreck: LootMap: unable to load WEAPON_ATTACHMENT", LogLevel.ERROR);
		
		if(!LoadSection(loadContext, SCR_EArsenalItemType.NON_LETHAL_THROWABLE))
			Print("TrainWreck: LootMap: unable to load NON_LETHAL_THROWABLE", LogLevel.ERROR);
		
		return true;
	}
	
	private bool LoadSection(notnull SCR_JsonLoadContext context, SCR_EArsenalItemType type)
	{		
		array<ref TW_LootConfigItem> items = {};
		string keyValue = SCR_TW_Util.ArsenalTypeAsString(type);
		
		bool success = context.ReadValue(keyValue, items);
		
		if(success)
			lootTable.Insert(type, items);
		else
			lootTable.Insert(type, {});
		
		return success;
	}
	
	override void OnGameModeStart()
	{
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		
		if(!rpl.IsMaster() && rpl.Role() != RplRole.Authority)
			return;
		
		GetGame().GetCallqueue().CallLater(InitializePlayerHub, SCR_TW_Util.FromSecondsToMilliseconds(1), false);
		InitializeLootMap();
		
		int globalCount = SCR_TW_InventoryLoot.GlobalLootContainers.Count();
		
		Print(string.Format("TrainWreck: Loot Containers -> %1", globalCount), LogLevel.NORMAL);
		
		int validCount = 0;
		foreach(auto container : SCR_TW_InventoryLoot.GlobalLootContainers)
			if(container)
				validCount++;
		
		Print(string.Format("TrainWreck: Valid(%1) Invalid(%2)", validCount, globalCount-validCount), LogLevel.WARNING);
		
		foreach(SCR_TW_InventoryLoot container : SCR_TW_InventoryLoot.GlobalLootContainers)
		{					
			if(!container) 
				continue;
			
			string format = string.Format("TrainWreck: %1", container.GetOwner().GetPrefabData().GetPrefabName());
			int spawnCount = Math.RandomIntInclusive(1, 6);
			
			// How many different things are we going to try spawning?								
			for(int i = 0; i < spawnCount; i++)
			{	
				string baseFormat = string.Format("%1\n\tFlags(%2)", format, container.GetTypeFlags());
				auto arsenalItem = GetRandomItemByFlag(container.GetTypeFlags());
				
				baseFormat += string.Format("%1\n\tSelected: %2", format, arsenalItem.resourceName);
				
				if(!arsenalItem)
				{
					Print(string.Format("%1\n\tSelected loot item is null", baseFormat), LogLevel.ERROR);		
					break;
				}				
				
				// Are we going to spawn the selected item?
				float seedPercentage = Math.RandomFloat(0.001, 100);
				if(arsenalItem.chanceToSpawn > seedPercentage)
				{
					Print(string.Format("%1: Skipping (no chance): %2", baseFormat, arsenalItem.resourceName), LogLevel.WARNING);
					continue;
				}
				
				// Add item a random amount of times to the container based on settings
				int itemCount = Math.RandomIntInclusive(1, arsenalItem.randomSpawnCount);
				bool tryAgain = false;
				for(int x = 0; x < itemCount; x++)
				{
					bool success = container.InsertItem(arsenalItem);
					
					if(!success)
					{
						tryAgain = true;
						break;
					}
				}
				
				if(tryAgain)
					spawnCount--;
			}
		}
	}
	
	private void InitializeExtractionSites()
	{
		int randomCount = Math.RandomIntInclusive(1, numberOfExtractionSites);
		randomCount = Math.Min(randomCount, possibleExtractionSites.Count());
		
		int exclude = possibleExtractionSites.Count() - randomCount;
		for(int i = 0; i < exclude; i++)
			possibleExtractionSites.Remove(possibleExtractionSites.GetRandomIndex());
	}
	
	private void InitializePlayerHub()
	{
		if(possibleSpawnAreas.IsEmpty())
		{
			Print("TrainWreck: No spawn points have been registered", LogLevel.ERROR);
			return;
		}
		
		SCR_SiteSlotEntity site = possibleSpawnAreas.GetRandomElement();
		
		Resource hubResource = Resource.Load(playerHubPrefab);
		
		if(!hubResource.IsValid())
		{
			Print(string.Format("TrainWreck: Unable to spawn %1, invalid resource", playerHubPrefab), LogLevel.ERROR);
			return;
		}
		
		IEntity spawnSiteEntity = site.SpawnEntityInSlot(hubResource);
		
		if(!spawnSiteEntity)
		{
			Print(string.Format("TrainWreck: Was unable to spawn player hub. %1", playerHubPrefab), LogLevel.ERROR);
			return;
		}
		
		//GetGame().GetCallqueue().CallLater(DespawnPlayerSpawn, 1000 * 60 * playerHubDespawnTimerInMinutes, false, spawnSiteEntity);		
	}
	
	private void DespawnPlayerSpawn(IEntity entity)
	{
		SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}
	
	private void InitializeLootMap()
	{
		if(HasLootTable())
		{
			Print("TrainWreck: lootmap.json file discovered");
			IngestLootTableFromFile();
			return;
		}
		
		ref array<Faction> allFactions = {};
		GetGame().GetFactionManager().GetFactionsList(allFactions);
		
		SCR_EntityCatalog globalCatalog;
		
		foreach(auto currentFaction : allFactions)
		{
			auto faction = SCR_Faction.Cast(currentFaction);
			
			if(!faction)
				continue;
			
			auto factionCatalog = faction.GetFactionEntityCatalogOfType(EEntityCatalogType.ITEM);
			
			if(!globalCatalog)
				globalCatalog = factionCatalog;
			else
				globalCatalog.MergeCatalogs(factionCatalog);				
		}
		
		// Process the global catalog of items now
		ref array<SCR_EntityCatalogEntry> catalogItems = {};
		int entityCount = globalCatalog.GetEntityList(catalogItems);
		
		foreach(auto entry : catalogItems)
		{
			ref array<SCR_BaseEntityCatalogData> itemData = {};
			entry.GetEntityDataList(itemData);
			
			// We only care about fetching Arsenal Items
			foreach(auto data : itemData)
			{
				auto arsenalItem = SCR_ArsenalItem.Cast(data);
				
				if(!arsenalItem)	
					continue;
				
				if(!arsenalItem.IsEnabled())
					break;
				
				auto itemType = arsenalItem.GetItemType();
				auto itemMode = arsenalItem.GetItemMode();
				auto prefab = entry.GetPrefab();
				
				arsenalItem.SetItemPrefab(prefab);
				
				if(!arsenalItem)
				{
					Print("TrainWreck: Failed to create LootItem during initialization");
					break;	
				}
				
				if(lootMap.Contains(itemType))
					lootMap.Get(itemType).Insert(arsenalItem);
				else
				{
					lootMap.Insert(itemType, {});					
					lootMap.Get(itemType).Insert(arsenalItem);
				}				
			}
		}
		
		// Finally, we can write to json file!
		bool success = OutputLootTableFile();		
		if(!success)
			Print("TrainWreck: Failed to write lootmap.json", LogLevel.ERROR);
	}
	
	TW_LootConfigItem GetRandomItemByFlag(int type)
	{
		array<SCR_EArsenalItemType> selectedItems = {};
		
		if(type > 0)
		{
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.HEAL) && lootTable.Contains(SCR_EArsenalItemType.HEAL))
				selectedItems.Insert(SCR_EArsenalItemType.HEAL);
		
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.LEGS) && lootTable.Contains(SCR_EArsenalItemType.LEGS))
				selectedItems.Insert(SCR_EArsenalItemType.LEGS);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.TORSO) && lootTable.Contains(SCR_EArsenalItemType.TORSO))
				selectedItems.Insert(SCR_EArsenalItemType.TORSO);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.RIFLE) && lootTable.Contains(SCR_EArsenalItemType.RIFLE))
				selectedItems.Insert(SCR_EArsenalItemType.RIFLE);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.PISTOL) && lootTable.Contains(SCR_EArsenalItemType.PISTOL))
				selectedItems.Insert(SCR_EArsenalItemType.PISTOL);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.FOOTWEAR) && lootTable.Contains(SCR_EArsenalItemType.FOOTWEAR))
				selectedItems.Insert(SCR_EArsenalItemType.FOOTWEAR);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.BACKPACK) && lootTable.Contains(SCR_EArsenalItemType.BACKPACK))
				selectedItems.Insert(SCR_EArsenalItemType.BACKPACK);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.HEADWEAR) && lootTable.Contains(SCR_EArsenalItemType.HEADWEAR))
				selectedItems.Insert(SCR_EArsenalItemType.HEADWEAR);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.EQUIPMENT) && lootTable.Contains(SCR_EArsenalItemType.EQUIPMENT))
				selectedItems.Insert(SCR_EArsenalItemType.EQUIPMENT);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.EXPLOSIVES) && lootTable.Contains(SCR_EArsenalItemType.EXPLOSIVES))
				selectedItems.Insert(SCR_EArsenalItemType.EXPLOSIVES);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.MACHINE_GUN) && lootTable.Contains(SCR_EArsenalItemType.MACHINE_GUN))
				selectedItems.Insert(SCR_EArsenalItemType.MACHINE_GUN);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.SNIPER_RIFLE) && lootTable.Contains(SCR_EArsenalItemType.SNIPER_RIFLE))
				selectedItems.Insert(SCR_EArsenalItemType.SNIPER_RIFLE);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.RADIO_BACKPACK) && lootTable.Contains(SCR_EArsenalItemType.RADIO_BACKPACK))
				selectedItems.Insert(SCR_EArsenalItemType.RADIO_BACKPACK);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.VEST_AND_WAIST) && lootTable.Contains(SCR_EArsenalItemType.VEST_AND_WAIST))
				selectedItems.Insert(SCR_EArsenalItemType.VEST_AND_WAIST);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.ROCKET_LAUNCHER) && lootTable.Contains(SCR_EArsenalItemType.ROCKET_LAUNCHER))
				selectedItems.Insert(SCR_EArsenalItemType.ROCKET_LAUNCHER);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.LETHAL_THROWABLE) && lootTable.Contains(SCR_EArsenalItemType.LETHAL_THROWABLE))
				selectedItems.Insert(SCR_EArsenalItemType.LETHAL_THROWABLE);
					
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.WEAPON_ATTACHMENT) && lootTable.Contains(SCR_EArsenalItemType.WEAPON_ATTACHMENT))
				selectedItems.Insert(SCR_EArsenalItemType.WEAPON_ATTACHMENT);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.NON_LETHAL_THROWABLE) && lootTable.Contains(SCR_EArsenalItemType.NON_LETHAL_THROWABLE))
				selectedItems.Insert(SCR_EArsenalItemType.NON_LETHAL_THROWABLE);			
		}
		else
		{
			if(lootTable.Contains(SCR_EArsenalItemType.HEAL))
				selectedItems.Insert(SCR_EArsenalItemType.HEAL);
		
			if(lootTable.Contains(SCR_EArsenalItemType.LEGS))
				selectedItems.Insert(SCR_EArsenalItemType.LEGS);
			
			if(lootTable.Contains(SCR_EArsenalItemType.TORSO))
				selectedItems.Insert(SCR_EArsenalItemType.TORSO);
			
			if(lootTable.Contains(SCR_EArsenalItemType.RIFLE))
				selectedItems.Insert(SCR_EArsenalItemType.RIFLE);
			
			if(lootTable.Contains(SCR_EArsenalItemType.PISTOL))
				selectedItems.Insert(SCR_EArsenalItemType.PISTOL);
			
			if(lootTable.Contains(SCR_EArsenalItemType.FOOTWEAR))
				selectedItems.Insert(SCR_EArsenalItemType.FOOTWEAR);
			
			if(lootTable.Contains(SCR_EArsenalItemType.BACKPACK))
				selectedItems.Insert(SCR_EArsenalItemType.BACKPACK);
			
			if(lootTable.Contains(SCR_EArsenalItemType.HEADWEAR))
				selectedItems.Insert(SCR_EArsenalItemType.HEADWEAR);
			
			if(lootTable.Contains(SCR_EArsenalItemType.EQUIPMENT))
				selectedItems.Insert(SCR_EArsenalItemType.EQUIPMENT);
			
			if(lootTable.Contains(SCR_EArsenalItemType.EXPLOSIVES))
				selectedItems.Insert(SCR_EArsenalItemType.EXPLOSIVES);
			
			if(lootTable.Contains(SCR_EArsenalItemType.MACHINE_GUN))
				selectedItems.Insert(SCR_EArsenalItemType.MACHINE_GUN);
			
			if(lootTable.Contains(SCR_EArsenalItemType.SNIPER_RIFLE))
				selectedItems.Insert(SCR_EArsenalItemType.SNIPER_RIFLE);
			
			if(lootTable.Contains(SCR_EArsenalItemType.RADIO_BACKPACK))
				selectedItems.Insert(SCR_EArsenalItemType.RADIO_BACKPACK);
			
			if(lootTable.Contains(SCR_EArsenalItemType.VEST_AND_WAIST))
				selectedItems.Insert(SCR_EArsenalItemType.VEST_AND_WAIST);
			
			if(lootTable.Contains(SCR_EArsenalItemType.ROCKET_LAUNCHER))
				selectedItems.Insert(SCR_EArsenalItemType.ROCKET_LAUNCHER);
			
			if(lootTable.Contains(SCR_EArsenalItemType.LETHAL_THROWABLE))
				selectedItems.Insert(SCR_EArsenalItemType.LETHAL_THROWABLE);
					
			if(lootTable.Contains(SCR_EArsenalItemType.WEAPON_ATTACHMENT))
				selectedItems.Insert(SCR_EArsenalItemType.WEAPON_ATTACHMENT);
			
			if(lootTable.Contains(SCR_EArsenalItemType.NON_LETHAL_THROWABLE))
				selectedItems.Insert(SCR_EArsenalItemType.NON_LETHAL_THROWABLE);
		}
		
		return lootTable.Get(selectedItems.GetRandomElement()).GetRandomElement();
	}		
};