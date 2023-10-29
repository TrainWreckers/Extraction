class SCR_TW_InventoryLootClass : GameComponentClass {};

class SCR_TW_InventoryLoot : GameComponent
{
	[Attribute("0", UIWidgets.Flags, "Item Pool types to use", "", ParamEnumArray.FromEnum(SCR_EArsenalItemType))]
	private SCR_EArsenalItemType m_lootItemTypes;
	
	[Attribute("0", UIWidgets.Flags, "Item Mode types to use", "", ParamEnumArray.FromEnum(SCR_EArsenalItemMode))]
	private SCR_EArsenalItemMode m_lootItemModes;
	
	static SCR_EntityCatalog s_GlobalCatalog;	
	
	// These are dictionaries which shall help us perform lookups via flags versus 
	// having to query/sift through a catalog each time
	private static ref map<SCR_EArsenalItemType, ref array<SCR_EntityCatalogEntry>> s_ItemsByItemType = new map<SCR_EArsenalItemType, ref array<SCR_EntityCatalogEntry>>();
	private static ref map<SCR_EArsenalItemMode, ref array<SCR_EntityCatalogEntry>> s_ItemsByItemMode = new map<SCR_EArsenalItemMode, ref array<SCR_EntityCatalogEntry>>();
	
	
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
		Print(string.Format("TrainWreck Loot system has acquired %1 items.", entityCount), LogLevel.WARNING);
		
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
				
				if(s_ItemsByItemType.Contains(itemType))
					s_ItemsByItemType.Get(itemType).Insert(entry);
				else
				{
					s_ItemsByItemType.Insert(itemType, new ref array<SCR_EntityCatalogEntry>);
					s_ItemsByItemType.Get(itemType).Insert(entry);
				}
				
				if(s_ItemsByItemMode.Contains(itemMode))
					s_ItemsByItemMode.Get(itemMode).Insert(entry);
				else
				{
					s_ItemsByItemMode.Insert(itemMode, new ref array<SCR_EntityCatalogEntry>);
					s_ItemsByItemMode.Get(itemMode).Insert(entry);
				}								
			}
		}
		
		
	}		
	
	static int GetItemsByType(SCR_EArsenalItemType itemFlags, SCR_EArsenalItemMode itemModes, notnull array<SCR_EntityCatalogEntry> items)	
	{
		int count = 0;
		ref array<int> enumValues = new ref array<int>;			
		SCR_Enum.GetEnumValues(SCR_EArsenalItemType, enumValues);
			
		foreach(int enumValue : enumValues)
		{
			SCR_EArsenalItemType type = SCR_EArsenalItemType.HEAL;
				
			// Ensure we have something in the loot table for this item type
			if(!s_ItemsByItemType.Contains(type))
			{
				Print(string.Format("Sorry, couldn't locate anything in loot tables for %1", type), LogLevel.WARNING);
				continue;
			}
			
			foreach(SCR_EntityCatalogEntry entry : s_ItemsByItemType.Get(type))
			{
				
				if(itemModes != 0)
				{					
					
				}
				count++;
				items.Insert(entry);
			}
		}
			
		return count;
		
	}
	
	void SCR_TW_InventoryLoot(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		// If not playing the game exit
		if(!GetGame().InPlayMode())
			return;
		
		if(!s_GlobalCatalog)
			SInitializeCatalog();
		
		Print("This is working", LogLevel.WARNING);
		
		/*
		foreach(SCR_EntityCatalogEntry entry : catalogItems)
		{
			ref array<SCR_BaseEntityCatalogData> dataItems = new ref array<SCR_BaseEntityCatalogData>;
			entry.GetEntityDataList(dataItems);
			
			foreach(SCR_BaseEntityCatalogData data : dataItems)
			{
				SCR_ArsenalItem arsenalConfig = SCR_ArsenalItem.Cast(data);
				
				// This is an Arsenal configuration item
				if(arsenalConfig)
				{
					//arsenalConfig.GetItemType
				}
			}
		}*/
	}
};