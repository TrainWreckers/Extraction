sealed class TW_LootManager 
{
	// Provide the ability to grab 
	private static ref map<SCR_EArsenalItemType, ref array<ref TW_LootConfigItem>> s_LootTable = new ref map<SCR_EArsenalItemType, ref array<ref TW_LootConfigItem>>();
	
	// This should contain the resource names of all items that are valid for saving/loading 
	private static ref set<string> s_GlobalItems = new set<string>();
	
	private static ref array<SCR_EArsenalItemType> s_ArsenalItemTypes = {};
	
	static const string LootFileName = "$profile:lootmap.json";
	
	//! Is this resource in the global items set? - IF not --> invalid.
	static bool IsValidItem(ResourceName resource)
	{
		return s_GlobalItems.Contains(resource);
	}

	static void InitializeLootTable()
	{
		SCR_Enum.GetEnumValues(SCR_EArsenalItemType, s_ArsenalItemTypes);
		
		if(HasLootTable())
		{
			Print(string.Format("TrainWreck: Detected loot table %1", LootFileName));
			IngestLootTableFromFile();
			return;
		}
		
		ref map<SCR_EArsenalItemType, ref array<SCR_ArsenalItem>> lootMap = new map<SCR_EArsenalItemType, ref array<SCR_ArsenalItem>>();
		ref array<ref SCR_EntityCatalogMultiList> catalogConfigs = SCR_TW_ExtractionHandler.GetInstance().GetCatalogConfigs();
		
		SCR_EntityCatalog globalCatalog;
		foreach(SCR_EntityCatalogMultiList catalogList : catalogConfigs)
		{
			if(!globalCatalog)
			{
				globalCatalog = catalogList;
				continue;
			}
			globalCatalog.MergeCatalogs(catalogList);
		}
		
		ref array<SCR_EntityCatalogEntry> catalogItems = {};
		int entityCount = globalCatalog.GetEntityList(catalogItems);
		
		foreach(auto entry : catalogItems)
		{
			ref array<SCR_BaseEntityCatalogData> itemData = {};
			entry.GetEntityDataList(itemData);
			
			// We only care about fetching arsenal items 
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
					Print("TrainWreck: Failed to create LootItem config during initialization");
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
		
		bool success = OutputLootTableFile(lootMap);
		if(!success)
			Print(string.Format("TrainWreck: Failed to write %1", LootFileName), LogLevel.ERROR);
		IngestLootTableFromFile();
	}		
		
	static void SpawnLoot()
	{
		int globalCount = SCR_TW_InventoryLoot.GlobalLootContainers.Count();
		
		Print(string.Format("TrainWreck: Loot Containers -> %1", globalCount), LogLevel.NORMAL);
		
		int validCount = 0;
		foreach(auto container : SCR_TW_InventoryLoot.GlobalLootContainers)
			if(container)
				validCount++;
		
		Print(string.Format("TrainWreck: Valid(%1) Invalid(%2)", validCount, globalCount-validCount), LogLevel.WARNING);
		
		foreach(SCR_TW_InventoryLoot container : SCR_TW_InventoryLoot.GlobalLootContainers)
			SpawnLootInContainer(container);	
	}
	
	static void SpawnLootInContainer(SCR_TW_InventoryLoot container)
	{
		if(!container) 
			return;
			
		string format = string.Format("TrainWreck: %1", container.GetOwner().GetPrefabData().GetPrefabName());
		int spawnCount = Math.RandomIntInclusive(1, 6);
			
		// How many different things are we going to try spawning?								
		for(int i = 0; i < spawnCount; i++)
		{	
			auto arsenalItem = TW_LootManager.GetRandomByFlag(container.GetTypeFlags());
			
			if(!arsenalItem)
				break;
				
			// Are we going to spawn the selected item?
			float seedPercentage = Math.RandomFloat(0.001, 100);
			if(arsenalItem.chanceToSpawn > seedPercentage)
				continue;
				
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
	
	static TW_LootConfigItem GetRandomByFlag(int type)
	{
		array<SCR_EArsenalItemType> selectedItems = {};
		
		if(type <= 0)
			return null;
		
		foreach(SCR_EArsenalItemType itemType : s_ArsenalItemTypes)
			if(SCR_Enum.HasFlag(type, itemType) && s_LootTable.Contains(itemType))
				selectedItems.Insert(itemType);
		
		// Check if nothing was selected
		if(selectedItems.IsEmpty())
			return null;
		
		auto items = s_LootTable.Get(selectedItems.GetRandomElement());
		
		// Check if nothing was available
		if(!items || items.IsEmpty())
			return null;
		
		return items.GetRandomElement();
	}
	
	private static bool OutputLootTableFile(notnull map<SCR_EArsenalItemType, ref array<SCR_ArsenalItem>> lootMap)
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
		
		return saveContext.SaveToFile(LootFileName);
	}
	
	private static bool HasLootTable()
	{
		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		return loadContext.LoadFromFile(LootFileName);
	}
	
	private static bool IngestLootTableFromFile()
	{
		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		bool loadSuccess = loadContext.LoadFromFile(LootFileName);
		
		if(!loadSuccess)
		{
			Print("TrainWreck: Was unable to laod loot map. Please verify it exists, and has valid syntax");
			return false;
		}
		
		array<SCR_EArsenalItemType> itemTypes = {};
		SCR_Enum.GetEnumValues(SCR_EArsenalItemType, itemTypes);
		string name = string.Empty;
		
		foreach(SCR_EArsenalItemType itemType : itemTypes)
		{
			if(!LoadSection(loadContext, itemType))
			{
				name = SCR_Enum.GetEnumName(SCR_EArsenalItemType, itemType);
				Print(string.Format("TrainWreck: LootMap: Unable to load %1", name), LogLevel.ERROR);
			}
		}
		
		return true;
	}
	
	private static bool LoadSection(notnull SCR_JsonLoadContext context, SCR_EArsenalItemType type)
	{
		array<ref TW_LootConfigItem> items = {};
		string keyValue = SCR_TW_Util.ArsenalTypeAsString(type);
		
		bool success = context.ReadValue(keyValue, items);
		
		if(success)
			s_LootTable.Insert(type, items);
		else
			s_LootTable.Insert(type, {});
		
		// Ensure we're tracking the resource names for everything 
		foreach(ref TW_LootConfigItem item : items)
		{
			
			if(!item)
			{
				Print("TrainWreck: Failed item", LogLevel.ERROR);
				continue;
			}
			
			if(!s_GlobalItems.Contains(item.resourceName))
				s_GlobalItems.Insert(item.resourceName);
		}
		
		return true;
	}
};