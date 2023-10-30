class SCR_TW_InventoryLootClass : ScriptComponentClass {};

class SCR_TW_InventoryLoot : ScriptComponent
{
	[Attribute("0", UIWidgets.Flags, "Item Pool types to use", "", ParamEnumArray.FromEnum(SCR_EArsenalItemType))]
	private SCR_EArsenalItemType m_lootItemTypes;
	
	[Attribute("0", UIWidgets.Flags, "Item Mode types to use", "", ParamEnumArray.FromEnum(SCR_EArsenalItemMode))]
	private SCR_EArsenalItemMode m_lootItemModes;
	
	static SCR_EntityCatalog s_GlobalCatalog;	
	
	// These are dictionaries which shall help us perform lookups via flags versus 
	// having to query/sift through a catalog each time
	private static ref map<SCR_EArsenalItemType, ref array<ref SCR_TW_LootItem>> s_Items = new ref map<SCR_EArsenalItemType, ref array<ref SCR_TW_LootItem>>();	
	
	
	static void SInitializeCatalog()
	{
		ref array<Faction> allFactions = new array<Faction>;
		GetGame().GetFactionManager().GetFactionsList(allFactions);
		
		foreach(Faction currentFaction : allFactions)
		{
			SCR_Faction faction = SCR_Faction.Cast(currentFaction);
			
			if(!faction)
				continue;
			
			SCR_EntityCatalog factionCatalog = faction.GetFactionEntityCatalogOfType(EEntityCatalogType.ITEM);
			
			if(s_GlobalCatalog == null)
				s_GlobalCatalog = factionCatalog;
			else
				s_GlobalCatalog.MergeCatalogs(factionCatalog);
		}
		
		ref array<SCR_EntityCatalogEntry> catalogItems = new ref array<SCR_EntityCatalogEntry>;
		int entityCount = s_GlobalCatalog.GetEntityList(catalogItems);
		
		foreach(SCR_EntityCatalogEntry entry : catalogItems)
		{
			ref array<SCR_BaseEntityCatalogData> itemData = new ref array<SCR_BaseEntityCatalogData>;
			entry.GetEntityDataList(itemData);
			
			// We only care about fetching Arsenal Items 
			foreach(SCR_BaseEntityCatalogData data : itemData)
			{
				SCR_ArsenalItem arsenalItem = SCR_ArsenalItem.Cast(data);
				
				if(!arsenalItem)
					continue;
				
				SCR_EArsenalItemType itemType = arsenalItem.GetItemType();
				SCR_EArsenalItemMode itemMode = arsenalItem.GetItemMode();
				ResourceName prefab = entry.GetPrefab();
				
				SCR_TW_LootItem item = new SCR_TW_LootItem(entry.GetEntityName(), itemType, itemMode, prefab);
				
				if(!item)
				{
					Print("Item is null", LogLevel.ERROR);
					continue;
				}
				
				if(s_Items.Contains(itemType))
				{
					s_Items.Get(itemType).Insert(item);
					Print(string.Format("TrainWreck: Last item is null: %1", s_Items.Get(itemType)[s_Items.Get(itemType).Count()-1] == null), LogLevel.ERROR);
				}
				else
				{
					s_Items.Insert(itemType, new ref array<ref SCR_TW_LootItem>);
					s_Items.Get(itemType).Insert(item);
				}
			}
		}
		
		
	}		
	
	static void GetItemsForFlag(int type, notnull array<ref SCR_TW_LootItem> items)
	{
		if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.HEAL))
		{
			if(s_Items.Contains(SCR_EArsenalItemType.HEAL))
			{
				foreach(auto item : s_Items.Get(SCR_EArsenalItemType.HEAL))
					items.Insert(item);
			}
		}
		
		if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.LEGS))
		{
			if(s_Items.Contains(SCR_EArsenalItemType.LEGS))
				foreach(auto item : s_Items.Get(SCR_EArsenalItemType.LEGS))
					items.Insert(item);
		}
		
		if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.TORSO))
		{
			if(s_Items.Contains(SCR_EArsenalItemType.TORSO))
				foreach(auto item : s_Items.Get(SCR_EArsenalItemType.TORSO))
					items.Insert(item);
		}
		
		if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.RIFLE))
		{
			if(s_Items.Contains(SCR_EArsenalItemType.RIFLE))
				foreach(auto item : s_Items.Get(SCR_EArsenalItemType.RIFLE))
					items.Insert(item);
		}
		
		if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.PISTOL))
		{
			if(s_Items.Contains(SCR_EArsenalItemType.PISTOL))
			{
				foreach(auto item : s_Items.Get(SCR_EArsenalItemType.PISTOL))
					items.Insert(item);
			}
		}
		
		if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.FOOTWEAR))
		{
			if(s_Items.Contains(SCR_EArsenalItemType.FOOTWEAR))
				foreach(auto item : s_Items.Get(SCR_EArsenalItemType.FOOTWEAR))
					items.Insert(item);
		}
		
		if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.BACKPACK))
		{
			if(s_Items.Contains(SCR_EArsenalItemType.BACKPACK))
				foreach(auto item : s_Items.Get(SCR_EArsenalItemType.BACKPACK))
					items.Insert(item);
		}
		
		if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.HEADWEAR))
		{
			if(s_Items.Contains(SCR_EArsenalItemType.HEADWEAR))
				foreach(auto item: s_Items.Get(SCR_EArsenalItemType.HEADWEAR))
					items.Insert(item);
		}
		
		if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.EQUIPMENT))
		{
			if(s_Items.Contains(SCR_EArsenalItemType.EQUIPMENT))
				foreach(auto item : s_Items.Get(SCR_EArsenalItemType.EQUIPMENT))
					items.Insert(item);
		}
		
		if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.EXPLOSIVES))
		{
			if(s_Items.Contains(SCR_EArsenalItemType.EXPLOSIVES))
				foreach(auto item : s_Items.Get(SCR_EArsenalItemType.EXPLOSIVES))
					items.Insert(item);
		}
		
		if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.MACHINE_GUN))
		{
			if(s_Items.Contains(SCR_EArsenalItemType.MACHINE_GUN))
				foreach(auto item : s_Items.Get(SCR_EArsenalItemType.MACHINE_GUN))
					items.Insert(item);
		}
		
		if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.SNIPER_RIFLE))
		{
			if(s_Items.Contains(SCR_EArsenalItemType.SNIPER_RIFLE))
				foreach(auto item : s_Items.Get(SCR_EArsenalItemType.SNIPER_RIFLE))	
					items.Insert(item);
		}
		
		if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.RADIO_BACKPACK))
		{
			if(s_Items.Contains(SCR_EArsenalItemType.RADIO_BACKPACK))
				foreach(auto item : s_Items.Get(SCR_EArsenalItemType.RADIO_BACKPACK))
					items.Insert(item);
		}
		
		if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.VEST_AND_WAIST))
		{
			if(s_Items.Contains(SCR_EArsenalItemType.VEST_AND_WAIST))
				foreach(auto item : s_Items.Get(SCR_EArsenalItemType.VEST_AND_WAIST))
					items.Insert(item);
		}
		
		if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.ROCKET_LAUNCHER))
		{
			if(s_Items.Contains(SCR_EArsenalItemType.ROCKET_LAUNCHER))
				foreach(auto item : s_Items.Get(SCR_EArsenalItemType.ROCKET_LAUNCHER))
					items.Insert(item);
		}
		
		if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.LETHAL_THROWABLE))
		{
			if(s_Items.Contains(SCR_EArsenalItemType.LETHAL_THROWABLE))
				foreach(auto item : s_Items.Get(SCR_EArsenalItemType.LETHAL_THROWABLE))
					items.Insert(item);
		}
		
		if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.WEAPON_ATTACHMENT))
		{
			if(s_Items.Contains(SCR_EArsenalItemType.WEAPON_ATTACHMENT))
				foreach(auto item : s_Items.Get(SCR_EArsenalItemType.WEAPON_ATTACHMENT))
					items.Insert(item);
		}
		
		if(SCR_Enum.HasFlag(type, SCR_EArsenalItemType.NON_LETHAL_THROWABLE))		
		{
			if(s_Items.Contains(SCR_EArsenalItemType.NON_LETHAL_THROWABLE))
				foreach(auto item : s_Items.Get(SCR_EArsenalItemType.NON_LETHAL_THROWABLE))
					items.Insert(item);
		}				
	}	

	
	static int GetItemsByType(SCR_EArsenalItemType itemFlags, SCR_EArsenalItemMode itemModes, notnull array<ref SCR_TW_LootItem> items)	
	{
		int count = 0;
		
		// If no flags were selected we'll grab all the things
		if(itemFlags == 0)
		{
			foreach(SCR_EArsenalItemType type, ref array<ref SCR_TW_LootItem> lootItems : s_Items)
			{
				foreach(auto item : lootItems)
					items.Insert(item);
			}
		}
		// If a flag was selected we'll grab only the items that match the selected flags
		else
		{
			GetItemsForFlag(itemFlags, items);
			count = items.Count();
		}
					
		return count;		
	}
	
	override void OnPostInit(IEntity owner)
	{
		if(!GetGame().InPlayMode())
		{
			return;
		}
		
		
		if(!s_GlobalCatalog)
			SInitializeCatalog();
		
		InventoryStorageManagerComponent storageManager = InventoryStorageManagerComponent.Cast(owner.FindComponent(InventoryStorageManagerComponent));
		BaseUniversalInventoryStorageComponent inventoryComponent = BaseUniversalInventoryStorageComponent.Cast(owner.FindComponent(BaseUniversalInventoryStorageComponent));
		
		array<ref SCR_TW_LootItem> entries = new array<ref SCR_TW_LootItem>;
		int count = GetItemsByType(m_lootItemTypes, m_lootItemModes, entries);
		
		if(count == 0)
		{
			Print("TrainWreck: No entries for Loot System", LogLevel.ERROR);
			return;
		}
		
		if(count > 0 && entries[0] == null)
		{
			Print("TrainWreck: Null entries in loot system", LogLevel.ERROR);
			return;	
		}
		
		Print(string.Format("There are %1 items that were pulled in", count), LogLevel.WARNING);
		int spawnCount = SCR_TW_Util.random.RandInt(1, 3);
		
		for(int i = 0; i < spawnCount; i++)
		{
			SCR_TW_LootItem selectedItem = entries.GetRandomElement();
			
			if(!selectedItem.GetPrefab())
			{
				Print(string.Format("TrainWreck: The prefab cannot be null: %1", selectedItem.GetItemName()), LogLevel.ERROR);
				continue;
			}
			
			storageManager.TrySpawnPrefabToStorage(selectedItem.GetPrefab(), inventoryComponent);
		}
	}
};