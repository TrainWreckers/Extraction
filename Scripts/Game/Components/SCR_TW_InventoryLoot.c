class SCR_TW_InventoryLootClass : ScriptComponentClass {};

class SCR_TW_InventoryLoot : ScriptComponent
{
	[Attribute("0", UIWidgets.Flags, "Item Pool types to use", "", ParamEnumArray.FromEnum(SCR_EArsenalItemType))]
	private SCR_EArsenalItemType m_lootItemTypes;
	
	[Attribute("0", UIWidgets.Flags, "Item Mode types to use", "", ParamEnumArray.FromEnum(SCR_EArsenalItemMode))]
	private SCR_EArsenalItemMode m_lootItemModes;
	
	static ref array<SCR_TW_InventoryLoot> GlobalLootContainers = {};
	
	private InventoryStorageManagerComponent storageManager;
	private BaseUniversalInventoryStorageComponent storage;
	
	SCR_EArsenalItemType GetTypeFlags() { return m_lootItemTypes; }
	SCR_EArsenalItemMode GetModeFlags() { return m_lootItemModes; }
	InventoryStorageManagerComponent GetStorageManager() { return storageManager; }
	BaseUniversalInventoryStorageComponent GetStorage() { return storage; }
	
	override void OnPostInit(IEntity owner)
	{
		if(!GetGame().InPlayMode())
			return;				
		
		// For testing purposes the logs for host + client are hard to validate
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		
		if(!rpl)
		{
			Print("TrainWreck: InventoryLoot Container requires a RPL Component in order to function property", LogLevel.ERROR);
			return;
		}
		
		if(!rpl.IsMaster())
			return;
		
		if(!GlobalLootContainers.Contains(this))
			GlobalLootContainers.Insert(this);
				
		storageManager = InventoryStorageManagerComponent.Cast(owner.FindComponent(InventoryStorageManagerComponent));
		storage = BaseUniversalInventoryStorageComponent.Cast(owner.FindComponent(BaseUniversalInventoryStorageComponent));				
	}
	
	bool InsertItem(SCR_ArsenalItem item)
	{
		if(!item)
		{
			Print("TrainWreck: can't insert null item", LogLevel.ERROR);
			return false;
		}
		
		if(!item.GetItemPrefab())
		{
			Print(string.Format("TrainWreck: Invalid prefab. %1", item.GetItemResourceName()), LogLevel.ERROR);
			return false;
		}
				
		auto result = storageManager.TrySpawnPrefabToStorage(item.GetItemResourceName());
		
		if(!result)
		{
			Print(string.Format("TrainWreck: Failed to insert item: %1", item.GetItemResourceName()), LogLevel.ERROR);
		}
		
		return result;
	}
	
};