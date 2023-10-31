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
	
	override void OnGameModeStart()
	{
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		
		if(!rpl.IsMaster())
			return;
		
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
			int spawnCount = Math.RandomIntInclusive(0, 3);
			
			if(spawnCount > 0)
				Print(string.Format("TrainWreck: SpawnCount(%1)", spawnCount), LogLevel.WARNING);
								
			for(int i = 0; i < spawnCount; i++)
			{								
				auto arsenalItem = GetRandomItemByFlag(container.GetTypeFlags());
				
				Print(string.Format("TrainWreck: Selected(%1)", arsenalItem.GetItemResourceName()), LogLevel.WARNING);
				
				if(!arsenalItem)
				{
					Print("TrainWreck: Selected loot item is null", LogLevel.ERROR);					
					break;
				}				
				
				float seedPercentage = Math.RandomFloat(0.001, 100);
				if(arsenalItem.GetItemChanceToSpawn() < seedPercentage)
					continue;
				
				// Add item a random amount of times to the container based on settings
				int itemCount = Math.RandomIntInclusive(0, arsenalItem.GetItemMaxSpawnCount());
				for(int x = 0; x < itemCount; x++)
					container.InsertItem(arsenalItem);				
			}
		}
	}
	
	private void InitializeLootMap()
	{
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
	}
	
	SCR_ArsenalItem GetRandomItemByFlag(int type)
	{
		array<SCR_EArsenalItemType> selectedItems = {};
		
		if(type != 0)
		{
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.HEAL) && lootMap.Contains(SCR_EArsenalItemType.HEAL))
				selectedItems.Insert(SCR_EArsenalItemType.HEAL);
		
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.LEGS) && lootMap.Contains(SCR_EArsenalItemType.LEGS))
				selectedItems.Insert(SCR_EArsenalItemType.LEGS);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.TORSO) && lootMap.Contains(SCR_EArsenalItemType.TORSO))
				selectedItems.Insert(SCR_EArsenalItemType.TORSO);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.RIFLE) && lootMap.Contains(SCR_EArsenalItemType.RIFLE))
				selectedItems.Insert(SCR_EArsenalItemType.RIFLE);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.PISTOL) && lootMap.Contains(SCR_EArsenalItemType.PISTOL))
				selectedItems.Insert(SCR_EArsenalItemType.PISTOL);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.FOOTWEAR) && lootMap.Contains(SCR_EArsenalItemType.FOOTWEAR))
				selectedItems.Insert(SCR_EArsenalItemType.FOOTWEAR);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.BACKPACK) && lootMap.Contains(SCR_EArsenalItemType.BACKPACK))
				selectedItems.Insert(SCR_EArsenalItemType.BACKPACK);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.HEADWEAR) && lootMap.Contains(SCR_EArsenalItemType.HEADWEAR))
				selectedItems.Insert(SCR_EArsenalItemType.HEADWEAR);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.EQUIPMENT) && lootMap.Contains(SCR_EArsenalItemType.EQUIPMENT))
				selectedItems.Insert(SCR_EArsenalItemType.EQUIPMENT);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.EXPLOSIVES) && lootMap.Contains(SCR_EArsenalItemType.EXPLOSIVES))
				selectedItems.Insert(SCR_EArsenalItemType.EXPLOSIVES);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.MACHINE_GUN) && lootMap.Contains(SCR_EArsenalItemType.MACHINE_GUN))
				selectedItems.Insert(SCR_EArsenalItemType.MACHINE_GUN);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.SNIPER_RIFLE) && lootMap.Contains(SCR_EArsenalItemType.SNIPER_RIFLE))
				selectedItems.Insert(SCR_EArsenalItemType.SNIPER_RIFLE);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.RADIO_BACKPACK) && lootMap.Contains(SCR_EArsenalItemType.RADIO_BACKPACK))
				selectedItems.Insert(SCR_EArsenalItemType.RADIO_BACKPACK);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.VEST_AND_WAIST) && lootMap.Contains(SCR_EArsenalItemType.VEST_AND_WAIST))
				selectedItems.Insert(SCR_EArsenalItemType.VEST_AND_WAIST);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.ROCKET_LAUNCHER) && lootMap.Contains(SCR_EArsenalItemType.ROCKET_LAUNCHER))
				selectedItems.Insert(SCR_EArsenalItemType.ROCKET_LAUNCHER);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.LETHAL_THROWABLE) && lootMap.Contains(SCR_EArsenalItemType.LETHAL_THROWABLE))
				selectedItems.Insert(SCR_EArsenalItemType.LETHAL_THROWABLE);
					
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.WEAPON_ATTACHMENT) && lootMap.Contains(SCR_EArsenalItemType.WEAPON_ATTACHMENT))
				selectedItems.Insert(SCR_EArsenalItemType.WEAPON_ATTACHMENT);
			
			if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.NON_LETHAL_THROWABLE) && lootMap.Contains(SCR_EArsenalItemType.NON_LETHAL_THROWABLE))
				selectedItems.Insert(SCR_EArsenalItemType.NON_LETHAL_THROWABLE);			
		}
		else
		{
			if(lootMap.Contains(SCR_EArsenalItemType.HEAL))
				selectedItems.Insert(SCR_EArsenalItemType.HEAL);
		
			if(lootMap.Contains(SCR_EArsenalItemType.LEGS))
				selectedItems.Insert(SCR_EArsenalItemType.LEGS);
			
			if(lootMap.Contains(SCR_EArsenalItemType.TORSO))
				selectedItems.Insert(SCR_EArsenalItemType.TORSO);
			
			if(lootMap.Contains(SCR_EArsenalItemType.RIFLE))
				selectedItems.Insert(SCR_EArsenalItemType.RIFLE);
			
			if(lootMap.Contains(SCR_EArsenalItemType.PISTOL))
				selectedItems.Insert(SCR_EArsenalItemType.PISTOL);
			
			if(lootMap.Contains(SCR_EArsenalItemType.FOOTWEAR))
				selectedItems.Insert(SCR_EArsenalItemType.FOOTWEAR);
			
			if(lootMap.Contains(SCR_EArsenalItemType.BACKPACK))
				selectedItems.Insert(SCR_EArsenalItemType.BACKPACK);
			
			if(lootMap.Contains(SCR_EArsenalItemType.HEADWEAR))
				selectedItems.Insert(SCR_EArsenalItemType.HEADWEAR);
			
			if(lootMap.Contains(SCR_EArsenalItemType.EQUIPMENT))
				selectedItems.Insert(SCR_EArsenalItemType.EQUIPMENT);
			
			if(lootMap.Contains(SCR_EArsenalItemType.EXPLOSIVES))
				selectedItems.Insert(SCR_EArsenalItemType.EXPLOSIVES);
			
			if(lootMap.Contains(SCR_EArsenalItemType.MACHINE_GUN))
				selectedItems.Insert(SCR_EArsenalItemType.MACHINE_GUN);
			
			if(lootMap.Contains(SCR_EArsenalItemType.SNIPER_RIFLE))
				selectedItems.Insert(SCR_EArsenalItemType.SNIPER_RIFLE);
			
			if(lootMap.Contains(SCR_EArsenalItemType.RADIO_BACKPACK))
				selectedItems.Insert(SCR_EArsenalItemType.RADIO_BACKPACK);
			
			if(lootMap.Contains(SCR_EArsenalItemType.VEST_AND_WAIST))
				selectedItems.Insert(SCR_EArsenalItemType.VEST_AND_WAIST);
			
			if(lootMap.Contains(SCR_EArsenalItemType.ROCKET_LAUNCHER))
				selectedItems.Insert(SCR_EArsenalItemType.ROCKET_LAUNCHER);
			
			if(lootMap.Contains(SCR_EArsenalItemType.LETHAL_THROWABLE))
				selectedItems.Insert(SCR_EArsenalItemType.LETHAL_THROWABLE);
					
			if(lootMap.Contains(SCR_EArsenalItemType.WEAPON_ATTACHMENT))
				selectedItems.Insert(SCR_EArsenalItemType.WEAPON_ATTACHMENT);
			
			if(lootMap.Contains(SCR_EArsenalItemType.NON_LETHAL_THROWABLE))
				selectedItems.Insert(SCR_EArsenalItemType.NON_LETHAL_THROWABLE);
		}
		
		return lootMap.Get(selectedItems.GetRandomElement()).GetRandomElement();
	}		
};