class SCR_TW_PlayerCrateComponentClass : ScriptComponentClass {};

class SCR_TW_PlayerCrateComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.Slider, params: "0 100, 1")]
	private int playerId;
	
	[Attribute("5", UIWidgets.Slider, params: "1 120 5", desc: "Timer (in minutes), crate will be deleted")]
	private int deleteTimer;
	
	private bool initialized = false;
	
	override void OnPostInit(IEntity owner)
	{
		if(!GetGame().InPlayMode())
			return;
		
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!(rpl && rpl.IsMaster() && rpl.Role() == RplRole.Authority))
			return;
		
		SCR_TW_ExtractionHandler.GetInstance().RegisterPlayerLoadoutCrate(this);
	}
	
	int GetPlayerId() { return playerId; }
	bool CanOpen(int playerId) { return this.playerId == playerId; }
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	private void RpcDo_SaveCrateContents()
	{
		SaveCrateContents_L();
	}
	
	void SaveCrateContents_S()
	{
		Rpc(RpcDo_SaveCrateContents);
	}
	
	private void SaveCrateContents_L()
	{
		SCR_TW_ExtractionHandler.GetInstance().UnregisterPlayerLoadoutCrate(this, playerId);
		
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		string filename = string.Format("$profile:%1.json", playerName);
		
		ref map<string, int> inventory = new map<string, int>();		
		SCR_JsonSaveContext saveContext = new SCR_JsonSaveContext();
		
		ref array<IEntity> items = {};
		InventoryStorageManagerComponent storageManager = InventoryStorageManagerComponent.Cast(GetOwner().FindComponent(InventoryStorageManagerComponent));		
		int itemCount = storageManager.GetItems(items);
		
		Print(string.Format("TrainWreck: There are %1 items in %2's crate", itemCount, playerName), LogLevel.ERROR);
		
		foreach(IEntity item : items)
		{
			ResourceName itemResource = item.GetPrefabData().GetPrefab().GetResourceName();
			
			if(!SCR_TW_ExtractionHandler.GetInstance().IsValidItem(itemResource))
			{
				Print(string.Format("TrainWreck: %1 is not savable", itemResource), LogLevel.WARNING);
				continue;
			}
			
			Print(string.Format("TrainWreck: %1 has %2", playerName, itemResource), LogLevel.WARNING);
			if(inventory.Contains(itemResource))
				inventory.Set(itemResource, inventory.Get(itemResource) + 1);
			else
				inventory.Insert(itemResource, 1);
		}
		
		saveContext.WriteValue("playerName", playerName);
		saveContext.WriteValue("items", inventory);
		
		bool saveSuccess = saveContext.SaveToFile(filename);
		if(!saveSuccess)
		{
			Print(string.Format("TrainWreck: Failed to save %1's crate to %2", playerName, filename), LogLevel.ERROR);
			return;
		}
		
		SCR_NotificationsComponent.SendToPlayer(playerId, ENotification.PLAYER_LOADOUT_SAVED);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	private void RpcDo_UpdatePlayerId(int playerId)
	{
		this.playerId = playerId;
	}
	
	void InitializeForPlayer(int playerId)
	{
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		if (!(rpl && rpl.Role() == RplRole.Authority))
			return;
		
		this.playerId = playerId;
		Rpc(RpcDo_UpdatePlayerId, playerId);
		
		ClearInventory();
		InitializePlayerInventory();
		GetGame().GetCallqueue().CallLater(SaveAndDeleteCrate, SCR_TW_Util.FromMinutesToMilliseconds(deleteTimer), false);
	}
	
	private void SaveAndDeleteCrate()
	{
		SCR_TW_ExtractionHandler.GetInstance().SaveAndDeleteCrate(GetPlayerId());
	}
	
	private void ClearInventory()
	{
		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(GetOwner().FindComponent(InventoryStorageManagerComponent));
		BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(GetOwner().FindComponent(BaseInventoryStorageComponent));
		ref array<IEntity> items = {};
		manager.GetItems(items);
		
		foreach(IEntity item : items)
		{
			manager.TryRemoveItemFromStorage(item, storage);
			SCR_EntityHelper.DeleteEntityAndChildren(item);
		}
	}
	
	private void InitializePlayerInventory()
	{		
		if(playerId < 0)
			return;
		
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		string filename = string.Format("$profile:%1.json", playerName);
		
		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		if(!loadContext.LoadFromFile(filename))
		{
			Print(string.Format("TrainWreck: Either %1 does not have a persistent file or it failed loading", playerName), LogLevel.WARNING);
			return;
		}
		
		ref map<string, int> items;		
		bool ingestSuccess = loadContext.ReadValue("items", items);
		
		if(!ingestSuccess)
		{
			Print(string.Format("TrainWreck: failed to load item map for %1 from json file", playerName), LogLevel.ERROR);
			return;
		}
		
		InventoryStorageManagerComponent manager = InventoryStorageManagerComponent.Cast(GetOwner().FindComponent(InventoryStorageManagerComponent));
		
		if(!manager)
		{
			Print(string.Format("TrainWreck: PlayerCrateComponent requires an InventoryStorageManagerComponent: %1", playerName), LogLevel.ERROR);
			return;
		}
		
		BaseInventoryStorageComponent storage = BaseInventoryStorageComponent.Cast(GetOwner().FindComponent(BaseInventoryStorageComponent));
		
		Print("TrainWreck: Populating Player Loot Box", LogLevel.NORMAL);
		foreach(string name, int amount : items)
		{			
			Print(string.Format("TrainWreck: Inserting %1 (%2)", name, amount), LogLevel.NORMAL);
			
			for(int i = 0; i < amount; i++)
			{				
				bool success = manager.TrySpawnPrefabToStorage(name, storage, purpose: EStoragePurpose.PURPOSE_DEPOSIT);
			}
		}
			
	}
};