[BaseContainerProps(configRoot: true)]
class TW_ProbabilityItem
{
	[Attribute("0", UIWidgets.CheckBox, category: "Probability")]
	private bool m_UseChance;
	
	[Attribute("0.5", UIWidgets.Slider, category: "Probability", params: "0.01 1 0.01")]
	private float m_Chance;
	
	[Attribute("", UIWidgets.ResourcePickerThumbnail, params: "et", category: "Prefabs")]
	private ref array<ResourceName> m_Prefabs;
	
	[Attribute("", UIWidgets.Slider, params: "0 10 1", category: "Probability")]
	private int m_RandomAmount;
	
	ResourceName GetRandomPrefab() { return m_Prefabs.GetRandomElement(); }
		
	bool UseChance() { return m_UseChance; }
	bool RollChance() { return Math.RandomFloat01() <= m_Chance; }
	int RandomCount() { return Math.RandomIntInclusive(0, m_RandomAmount); }
}

[BaseContainerProps(configRoot: true)]
class TW_InventoryWeaponConfig : TW_ProbabilityItem
{		
	[Attribute("2", UIWidgets.Slider, params: "1 10 1", category: "Ammo")]
	private int m_MinimumMagazines;
	
	[Attribute("5", UIWidgets.Slider, params: "1 10 1", category: "Ammo")]
	private int m_MaximumMagazines;
	
	int GetRandomMagCount() { return Math.RandomIntInclusive(m_MinimumMagazines, m_MaximumMagazines); }	
}

[BaseContainerProps(configRoot: true)]
class TW_InventoryConfig
{
	[Attribute("", UIWidgets.Auto, params: "conf class=TW_InventoryWeaponConfig", desc: "Probability config for primary weapon. This will avoid chance to spawn")]
	private ref TW_InventoryWeaponConfig m_PrimaryWeaponConfig;
	
	[Attribute("", UIWidgets.Auto, params: "conf class=TW_InventoryWeaponConfig", desc: "Probability config for secondary weapon")]
	private ref TW_InventoryWeaponConfig m_SecondaryWeaponConfig;
	
	[Attribute("", UIWidgets.Auto, params: "conf class=TW_ProbabilityItem", desc: "List of equipment items that should get added (if applicable)")]
	private ref array<ref TW_ProbabilityItem> m_EquipmentConfigs;
	
	[Attribute("", UIWidgets.Auto, params: "conf class=TW_ProbabilityItem")]
	private ref array<ref TW_ProbabilityItem> m_ItemConfigs;
	
	private ResourceName GetMagazineFromWeapon(TW_InventoryWeaponConfig config, SCR_InventoryStorageManagerComponent storageManager, SCR_CharacterControllerComponent controller = null)
	{
		// primary weapon should be guaranteed to spawn. It will ignore the chance system 
		ResourceName weaponResourceName = config.GetRandomPrefab();
		EntitySpawnParams params = EntitySpawnParams();
		storageManager.GetOwner().GetTransform(params.Transform);
		
		Resource weaponResource = Resource.Load(weaponResourceName);
		if(!weaponResource.IsValid())
		{
			Print(string.Format("TrainWreck: Invalid weapon resource: %1", weaponResourceName), LogLevel.ERROR);
			return ResourceName.Empty;
		}
		
		IEntity weapon = GetGame().SpawnEntityPrefab(weaponResource, GetGame().GetWorld(), params);
		
		if(!weapon)
		{
			Print(string.Format("TrainWreck: Was unable to spawn weapon %1", weaponResourceName), LogLevel.ERROR);
			return ResourceName.Empty;
		}
		
		WeaponComponent weaponComp = TW<WeaponComponent>.Find(weapon);
		
		
		BaseMagazineComponent comp = weaponComp.GetCurrentMagazine();
		
		if(!comp)
		{
			Print(string.Format("TrainWreck: Weapon does not have a weapon component %1", weaponResourceName), LogLevel.ERROR);
			return ResourceName.Empty;
		}
		
		if(!storageManager.TryInsertItem(weapon, EStoragePurpose.PURPOSE_WEAPON_PROXY))
		{
			Print(string.Format("TrainWreck: was unable to insert weapon: %1", weaponResourceName), LogLevel.ERROR);
			SCR_EntityHelper.DeleteEntityAndChildren(weapon);
		}
		
		if(controller)
		{
			controller.TryEquipRightHandItem(weapon, EEquipItemType.EEquipTypeWeapon);
		}
		
		return comp.GetOwner().GetPrefabData().GetPrefab().GetResourceName();
	}
	
	private bool SpawnItem(ResourceName resource, SCR_InventoryStorageManagerComponent storageManager)
	{
		if(resource == ResourceName.Empty)
			return false;
		
		Resource r = Resource.Load(resource);
		if(!r.IsValid())
			return false;
		
		EntitySpawnParams params = EntitySpawnParams();
		storageManager.GetOwner().GetTransform(params.Transform);
		
		IEntity item = GetGame().SpawnEntityPrefab(r, GetGame().GetWorld(), params);
		
		if(storageManager.CanInsertItem(item, EStoragePurpose.PURPOSE_ANY))
		{
			bool success = storageManager.TryInsertItem(item, EStoragePurpose.PURPOSE_ANY);		
		
			// The check for "can insert item" should alleviate the need for this 
			// but just in case
			if(!success)
				SCR_EntityHelper.DeleteEntityAndChildren(item);
			
			return success;
		}
		
		return false;
	}
	
	void ProvideInventory(SCR_InventoryStorageManagerComponent storageManager, SCR_CharacterControllerComponent controller)
	{	
		// Equipment must spawn first because that'll be used for storing weapons and items / etc	
		if(!m_EquipmentConfigs.IsEmpty())
		{
			foreach(TW_ProbabilityItem item : m_EquipmentConfigs)
				if(!item.UseChance() || (item.UseChance() && item.RollChance()))
					if(!SpawnItem(item.GetRandomPrefab(), storageManager))
						break;
		}
		
		// Primary weapon stuff 
		ResourceName primaryWeaponMag = GetMagazineFromWeapon(m_PrimaryWeaponConfig, storageManager, controller);
		
		if(primaryWeaponMag != ResourceName.Empty)
		{
			int magCount = m_PrimaryWeaponConfig.GetRandomMagCount();
			Print(string.Format("TrainWreck: %1 mags for %2", magCount, primaryWeaponMag), LogLevel.WARNING);
			
			for(int i = 0; i < magCount; i++)
				if(!SpawnItem(primaryWeaponMag, storageManager))
					break;
		}
		
		// Only work with secondary weapons if they are provided
		if(m_SecondaryWeaponConfig)
		{
			if(!m_SecondaryWeaponConfig.UseChance() || (m_SecondaryWeaponConfig.UseChance() && m_SecondaryWeaponConfig.RollChance()))
			{
				ResourceName secondaryWeaponMag = GetMagazineFromWeapon(m_SecondaryWeaponConfig, storageManager);
				if(secondaryWeaponMag)
				{
					int magCount = m_SecondaryWeaponConfig.GetRandomMagCount();
					for(int i = 0; i < magCount; i++)
						if(!SpawnItem(secondaryWeaponMag, storageManager))
							break;
				}
			}
		}		
		
		if(!m_ItemConfigs.IsEmpty())
		{
			foreach(TW_ProbabilityItem item : m_ItemConfigs)
				if(!item.UseChance() || (item.UseChance() && item.RollChance()))
				{
					int spawnCount = item.RandomCount();
				
					for(int i = 0; i < spawnCount; i++)
						if(!SpawnItem(item.GetRandomPrefab(), storageManager))
							break;
				}
		}
		
	}
}

class SCR_TW_RandomInventoryComponentClass : ScriptComponentClass {};

class SCR_TW_RandomInventoryComponent : ScriptComponent
{
	[Attribute("{A264CF3B7AF6405D}Configs/RandomLoadouts/Default.conf", params: "conf class=TW_InventoryConfig", category: "Loadout", UIWidgets.Auto)]
	private ref TW_InventoryConfig m_Config;
	private BaseWeaponManagerComponent weaponManager;
	private RplComponent rplComponent;
	private SCR_CharacterControllerComponent controller;
	
	override void OnPostInit(IEntity owner)
	{
		if(!GetGame().InPlayMode())
			return;
		
		if(!TW_Global.IsServer(owner))
		{
			Print("TrainWreck: Not server, no spawn shit", LogLevel.ERROR);
			return;
		}
		
		weaponManager = TW<BaseWeaponManagerComponent>.Find(owner);
		rplComponent = TW<RplComponent>.Find(owner);
		controller = TW<SCR_CharacterControllerComponent>.Find(GetOwner());
		
		if(m_Config)
			GetGame().GetCallqueue().CallLater(InitializeLoadout, 1000, false);
	}
	
	private void InitializeLoadout()
	{
		if(!TW_Global.IsServer(GetOwner()))
			return;
		
		SCR_InventoryStorageManagerComponent storageManager = TW<SCR_InventoryStorageManagerComponent>.Find(GetOwner());
		RplComponent rplComp = TW<RplComponent>.Find(storageManager.GetOwner());
		
		if(!rplComp.IsMaster())
		{
			Print("TrainWreck: Unable to initialize AI inventory because I'm not the master", LogLevel.WARNING);
			return;
		}
		
		if(!storageManager)
			return;
		
		m_Config.ProvideInventory(storageManager, controller);						
	}
};