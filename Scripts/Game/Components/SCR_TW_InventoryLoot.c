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
			return;
		
		if(!rpl.IsMaster())
			return;
		
		if(!GlobalLootContainers.Contains(this))
			GlobalLootContainers.Insert(this);
				
		storageManager = InventoryStorageManagerComponent.Cast(owner.FindComponent(InventoryStorageManagerComponent));
		storage = BaseUniversalInventoryStorageComponent.Cast(owner.FindComponent(BaseUniversalInventoryStorageComponent));				
	}
	
	bool InsertItem(TW_LootConfigItem item)
	{
		if(!item)		
			return false;
		
		Resource prefabResource = Resource.Load(item.resourceName);
		
		if(!prefabResource.IsValid())
		{
			Print(string.Format("TrainWreck: Invalid resource. Cannot spawn %1", item.resourceName), LogLevel.ERROR);
			return false;
		}
		
		EntitySpawnParams params = EntitySpawnParams();
		GetOwner().GetTransform(params.Transform);
		
		IEntity spawnedItem = GetGame().SpawnEntityPrefab(prefabResource, GetGame().GetWorld(), params);
		
		if(!spawnedItem)
		{
			Print(string.Format("TrainWreck: Was unable to spawn %1", item.resourceName), LogLevel.ERROR);
			return false;
		}
		
		BaseWeaponComponent weapon = BaseWeaponComponent.Cast(spawnedItem.FindComponent(BaseWeaponComponent));
		
		if(weapon)
		{
			BaseMagazineComponent magazine = weapon.GetCurrentMagazine();
			
			if(magazine)
			{
				if(!SCR_TW_ExtractionHandler.GetInstance().ShouldSpawnMagazine())
				{
					SCR_EntityHelper.DeleteEntityAndChildren(magazine.GetOwner());
				}
				else
				{	
					int maxAmmo = magazine.GetMaxAmmoCount();
					int newCount = Math.RandomIntInclusive(0, maxAmmo);
					magazine.SetAmmoCount(newCount);				
				}			
			}			
		}
		else
		{
			BaseMagazineComponent magazine = BaseMagazineComponent.Cast(spawnedItem.FindComponent(BaseMagazineComponent));
		
			if(magazine)
			{
				int maxAmmo = magazine.GetMaxAmmoCount();
				float percent = SCR_TW_ExtractionHandler.GetInstance().GetRandomAmmoPercent();
				
				int ammo = Math.RandomIntInclusive(1, maxAmmo * percent);
				magazine.SetAmmoCount(Math.ClampInt(ammo, 0, maxAmmo));
			}
		}
		
		
		bool success = storageManager.TryInsertItemInStorage(spawnedItem, storage);
		
		// If it failed to add to stoarge we must delete it from the world
		if(!success)
			SCR_EntityHelper.DeleteEntityAndChildren(spawnedItem);
		
		return success;
	}
	
};