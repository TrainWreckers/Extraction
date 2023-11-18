class TW_ReplicationTestingComponentClass : SCR_BaseGameModeComponentClass {};

class TW_ReplicationTestingComponent : SCR_BaseGameModeComponent
{
	[Attribute("", UIWidgets.ResourcePickerThumbnail, category: "Groups", params: "et")]
	private ResourceName groupPrefab;
	
	[Attribute("", UIWidgets.ResourcePickerThumbnail, category: "Equipment", params: "et")]
	private ResourceName equipmentPrefab;		
	
	[Attribute(EStoragePurpose.PURPOSE_LOADOUT_PROXY.ToString(), UIWidgets.ComboBox, category: "Equipment", enums: ParamEnumArray.FromEnum(EStoragePurpose))]
	private EStoragePurpose equipmentStoragePurpose;
	
	[Attribute("", UIWidgets.ResourcePickerThumbnail, category: "Item", params: "et")]
	private ResourceName itemPrefab;		
	
	[Attribute(EStoragePurpose.PURPOSE_LOADOUT_PROXY.ToString(), UIWidgets.ComboBox, category: "Item", enums: ParamEnumArray.FromEnum(EStoragePurpose))]
	private EStoragePurpose storagePurpose;
	
		
	[Attribute("", UIWidgets.Auto, category: "Items", params: "conf class=TW_InventoryConfig")]
	private ref TW_InventoryConfig inventoryConfig;
	
	
	[Attribute("", UIWidgets.Auto, category: "Spawn Location Names")]
	private ref array<string> locationNames;

	[Attribute("", UIWidgets.EditBox, category: "Box Name")]
	private string boxName;
			
	private IEntity box;
	private ref array<IEntity> locations = {};
		
	override void OnGameModeStart()
	{
		super.OnGameModeStart();
		if(!TW_Global.IsInRuntime())
			return;
		
		if(!TW_Global.IsServer(GetOwner()))
			return;
		
		box = GetGame().FindEntity(boxName);
		
		
		// Get locations for our spawn testing 
		foreach(string locationName : locationNames)
		{
			IEntity location = GetGame().FindEntity(locationName);
			
			locations.Insert(location);
		}
		
		GetGame().GetCallqueue().CallLater(Spawn, 20000, false);
	}
	
	void Spawn()
	{
		SpawnGroup(groupPrefab, locations.Get(0), "Spawn_GroupWithSingleItem");
		SpawnGroup(groupPrefab, locations.Get(1), "Spawn_GroupWithRandomItems");
		
		if(box)
			Spawn_ItemsInBox();
	}
	
	private SCR_AIGroup SpawnGroup(ResourceName groupResource, IEntity location, string nextCall, float delay = 1000)
	{
		Resource resource = Resource.Load(groupResource);
		EntitySpawnParams params = EntitySpawnParams();
		location.GetTransform(params.Transform);
		
		SCR_AIGroup group = SCR_AIGroup.Cast(GetGame().SpawnEntityPrefab(resource, GetGame().GetWorld(), params));
		
		GetGame().GetCallqueue().CallLaterByName(this, nextCall, delay, false, group);
		return group;
	}
	
	void Spawn_ItemsInBox()
	{
		SCR_InventoryStorageManagerComponent storageManager = TW<SCR_InventoryStorageManagerComponent>.Find(box);
		BaseInventoryStorageComponent storage = TW<BaseInventoryStorageComponent>.Find(box);
		
		if(!storageManager.TrySpawnPrefabToStorage(equipmentPrefab, storage, purpose: EStoragePurpose.PURPOSE_DEPOSIT))
			Print(string.Format("Was unable to add %1 equipment prefab into box", equipmentPrefab), LogLevel.ERROR);
		
		if(!storageManager.TrySpawnPrefabToStorage(itemPrefab, storage, purpose: EStoragePurpose.PURPOSE_DEPOSIT))
			Print(string.Format("Was unable to add %1 item prefab into box", itemPrefab), LogLevel.ERROR);
	}
	
	void Spawn_GroupWithSingleItem(SCR_AIGroup group)
	{
		ref array<AIAgent> agents = {};
		group.GetAgents(agents);
		
		foreach(AIAgent agent : agents)
		{
			IEntity controlledEntity = agent.GetControlledEntity();
			
			SCR_InventoryStorageManagerComponent storageManager = TW<SCR_InventoryStorageManagerComponent>.Find(controlledEntity);
			
			if(!storageManager.TrySpawnPrefabToStorage(equipmentPrefab, purpose: equipmentStoragePurpose))
			{
				Print(string.Format("Was unable to insert %1 into AI as %2", equipmentPrefab, SCR_Enum.GetEnumName(EStoragePurpose, equipmentStoragePurpose)), LogLevel.ERROR);				
			}
			
			if(!storageManager.TrySpawnPrefabToStorage(itemPrefab, purpose: storagePurpose))
			{
				Print(string.Format("Was unable to isnert %1 into AI as %2", itemPrefab, SCR_Enum.GetEnumName(EStoragePurpose, storagePurpose)), LogLevel.ERROR);
			}
		}
	}
	
	void Spawn_GroupWithRandomItems(SCR_AIGroup group)
	{
		ref array<AIAgent> agents = {};
		group.GetAgents(agents);
		
		foreach(AIAgent agent : agents)
		{
			IEntity controlledEntity = agent.GetControlledEntity();
			
			SCR_InventoryStorageManagerComponent storageManager = TW<SCR_InventoryStorageManagerComponent>.Find(controlledEntity);
			SCR_CharacterControllerComponent controller = TW<SCR_CharacterControllerComponent>.Find(controlledEntity);
			inventoryConfig.ProvideInventory(storageManager, controller);
		}
	}
};