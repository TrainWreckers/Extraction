[ComponentEditorProps(category: "GameScripted/TrainWreck/Systems", description: "Handles inventory persistence")]
class SCR_TW_ExtractionPlayerInventoryComponentClass : SCR_BaseGameModeComponentClass 
{
	static override array<typename> Requires(IEntityComponentSource src)
	{
		array<typename> requires = new array<typename>;
		
		requires.Insert(SerializerInventoryStorageManagerComponent);
		requires.Insert(SCR_TW_ExtractionHandler);
		
		return requires;
	}
};

class SCR_TW_ExtractionPlayerInventoryComponent : SCR_BaseGameModeComponent
{
	protected static SCR_TW_ExtractionPlayerInventoryComponent s_Instance;
	
	protected bool m_LocalPlayerLoadoutAvailable;
	ref SCR_PlayerLoadoutData m_LocalLoadoutData;
	
	//-----
	static SCR_TW_ExtractionPlayerInventoryComponent GetInstance()
	{
		if(!s_Instance)
			s_Instance = SCR_TW_ExtractionPlayerInventoryComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_TW_ExtractionPlayerInventoryComponent));
		
		return s_Instance;
	}
	
	//------------------------------------------------------------------------------------------------
	//! RPC Call to server to ensure only the server udpates/saves inventory 
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcUpdatePlayerInventory(int playerId)
	{
		bool success = SavePlayerLoadout(playerId);
		
		if(success)
		{
			SCR_NotificationsComponent.SendToPlayer(playerId, ENotification.PLAYER_LOADOUT_SAVED);
			SCR_TW_ExtractionHandler.GetInstance().UpdatePlayerInventoryCrate(playerId);
		}
		else
			SCR_NotificationsComponent.SendToPlayer(playerId, ENotification.PLAYER_LOADOUT_NOT_SAVED);
	}
	
	//------------------------------------------------------------------------------------------------
	void UpdatePlayerInventory(int playerId)
	{
		Rpc(RpcUpdatePlayerInventory, playerId);
	}
		
	protected bool SavePlayerLoadout(int playerId)
	{		
		// First lets grab the player whose inventory we're trying to save 
		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		
		if(!entity)
		{
			Print(string.Format("TrainWreck: unable to save player inventory for ID: %1 - no controllable entity found", playerId), LogLevel.ERROR);
			return false;
		}
		
		// We'll be using the player name to save loadouts because it's more reliable than player Id
		// which could change between sessions. 
		string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
		
		SCR_CharacterInventoryStorageComponent inventory = SCR_CharacterInventoryStorageComponent.Cast(entity.FindComponent(SCR_CharacterInventoryStorageComponent));
		
		if(!inventory)
		{
			Print(string.Format("TrainWreck: %1 does not have a CharacterInventoryStorageCompnent", name), LogLevel.ERROR);
			return false;
		}
		
		ref array<SCR_UniversalInventoryStorageComponent> storages = {};
		inventory.GetStorages(storages);
		
		Print(string.Format("TrainWreck: %1 has %2 storages", name, storages.Count()), LogLevel.DEBUG);
		
		ref array<InventoryItemComponent> items = {};
		ref map<string, int> loadoutMap = new map<string, int>();
		inventory.GetOwnedItems(items);
		
		ref array<BaseInventoryStorageComponent> storageQueue = {};
		
		// This will actually grab all equippable inventories
		// Alice, backpack, clothing, etc 
		foreach(InventoryItemComponent item : items)
		{
			InventoryStorageSlot slot = item.GetParentSlot();
			if(!slot) continue;
			
			IEntity attachedEntity = slot.GetAttachedEntity();			
			if(!attachedEntity) continue;
			
			ResourceName prefabName = attachedEntity.GetPrefabData().GetPrefab().GetResourceName();
			
			if(!loadoutMap.Contains(prefabName))
				loadoutMap.Insert(prefabName, 1);
			else
				loadoutMap.Set(prefabName, loadoutMap.Get(prefabName) + 1);						
			
			ClothNodeStorageComponent clothNode = ClothNodeStorageComponent.Cast(attachedEntity.FindComponent(ClothNodeStorageComponent));
			
			if(clothNode)
			{
				ProcessNode(clothNode, loadoutMap);
				continue;
			}
			
			BaseInventoryStorageComponent substorage = BaseInventoryStorageComponent.Cast(attachedEntity.FindComponent(BaseInventoryStorageComponent));
			
			if(substorage)
			{
				ProcessStorage(substorage, loadoutMap);
				continue;
			}
			
		}
		
		BaseInventoryStorageComponent weaponStorage = inventory.GetWeaponStorage();
		ProcessStorage(weaponStorage, loadoutMap);				
		
		SCR_JsonSaveContext saveContext = new SCR_JsonSaveContext();
		saveContext.WriteValue("playerName", name);
		
		string filename = string.Format("$profile:%1.json", name);
		
		// Load anything that may have been saved prior 
		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		bool priorLoad = loadContext.LoadFromFile(filename);
		
		if(priorLoad)
		{
			ref map<string, int> saved;
			loadContext.ReadValue("items", saved);
			
			foreach(auto itemName, auto itemCount : saved)
				if(loadoutMap.Contains(itemName))
					loadoutMap.Set(itemName, loadoutMap.Get(itemName) + itemCount);
				else
					loadoutMap.Insert(itemName, itemCount);
		}
		
		saveContext.WriteValue("items", loadoutMap);
		
		bool success = saveContext.SaveToFile(filename);
		return success;
	}
	
	private void ProcessNode(ClothNodeStorageComponent node, out notnull map<string, int> loadout)
	{
		// This is an alice/vest item and contains multiple nodes/items within it 
		int slotsCount = node.GetSlotsCount();
		
		for(int i = 0; i < slotsCount; i++)
		{
			InventoryStorageSlot slot = node.GetSlot(i);
			
			if(!slot) 
				continue;
			
			IEntity entity = slot.GetAttachedEntity();
			
			if(!entity)
				continue;
			
			BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(entity.FindComponent(BaseInventoryStorageComponent));
			
			if(!storage)
				continue;
			
			ProcessStorage(storage, loadout);
		}
	}
	
	private void ProcessStorage(BaseInventoryStorageComponent storage, out notnull map<string, int> loadout)
	{
		int slotsCount = storage.GetSlotsCount();
		
		for(int i = 0; i < slotsCount; i++)
		{
			InventoryStorageSlot slot = storage.GetSlot(i);
			
			if(!slot) 
				continue;
			
			IEntity attachedEntity = slot.GetAttachedEntity();
			
			if(!attachedEntity)
				continue;
			
			ResourceName resource = attachedEntity.GetPrefabData().GetPrefab().GetResourceName();
			
			if(!resource || resource.IsEmpty())
				continue;
			
			if(!loadout.Contains(resource))
				loadout.Insert(resource, 1);
			else
				loadout.Set(resource, loadout.Get(resource) + 1);
			
			BaseInventoryStorageComponent substorage = BaseInventoryStorageComponent.Cast(attachedEntity.FindComponent(BaseInventoryStorageComponent));
			
			if(substorage)
			{
				Print(string.Format("TrainWreck: %1: %2", substorage.ClassName(), attachedEntity.GetPrefabData().GetPrefabName()), LogLevel.DEBUG);
			}
		}
	}
	
	override void EOnInit(IEntity owner)
	{
		if(!GetGame().InPlayMode())
			return;
		
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		if (!(rpl && rpl.Role() == RplRole.Authority))
			return;
		
		SCR_BaseGameMode gameMode = SCR_BaseGameMode.Cast(GetGame().GetGameMode());
		gameMode.GetOnPlayerConnected().Insert(OnPlayerConnected);
	}
		
	override void OnPlayerConnected(int playerId)
	{
		super.OnPlayerConnected(playerId);
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		
		if(playerName == string.Empty)
		{
			Print(string.Format("TrainWreck (Inventory): Failed to get player name for playerId: %1", playerId), LogLevel.ERROR);
			return;
		}
		
	}
};