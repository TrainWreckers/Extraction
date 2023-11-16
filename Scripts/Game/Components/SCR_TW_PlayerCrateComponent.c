class SCR_TW_PlayerCrateComponentClass : ScriptComponentClass {};

class SCR_TW_PlayerCrateComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.Slider, params: "0 100, 1")]
	private int playerId;
	
	[Attribute("5", UIWidgets.Slider, params: "1 120 5", desc: "Timer (in minutes), crate will be deleted")]
	private int deleteTimer;
	
	int GetPlayerId() { return playerId; }
	bool CanOpen(int id) { return this.playerId == id; }
	
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
		InventoryStorageManagerComponent storageManager = TW<InventoryStorageManagerComponent>.Find(GetOwner());
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
	private void RpcDo_UpdatePlayerId(int id)
	{
		this.playerId = id;
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	protected void RpcDo_UpdateCrateInfo(RplId id, int playerId)
	{
		UpdatePlayerCrateInformation(id, playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	IEntity GetProviderFromRplId(RplId rplProviderId)
	{
		RplComponent rplComp = RplComponent.Cast(Replication.FindItem(rplProviderId));
		if (!rplComp)
			return null;

		return rplComp.GetEntity();
	}
	
	//! We want to grab the box across the network via same network ID 
	private void UpdatePlayerCrateInformation(RplId id, int playerId)
	{				
		this.playerId = playerId;
		
		Print("TrainWreck: I AM RPC'ing", LogLevel.ERROR);	
		IEntity box = GetProviderFromRplId(id);
		
		if(!box)
		{
			Print(string.Format("TrainWreck: LootBox RPC - unable to find box with ID: %1", playerId), LogLevel.ERROR);
			return;
		}
		
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		string name = string.Format("%1's Crate", playerName);
		string description = string.Format("This crate belongs to %1", playerName);
		
		SCR_TW_PlayerCrateComponent crate = TW<SCR_TW_PlayerCrateComponent>.Find(box);
		
		if(!crate)
		{
			Print(string.Format("TrainWreck: LootBox RPC - unable to find SCR_TW_PlayerCrateComponent on ID %1", playerId), LogLevel.ERROR);			
			return;
		}
		
		SCR_UniversalInventoryStorageComponent universal = TW<SCR_UniversalInventoryStorageComponent>.Find(crate.GetOwner());
		if(universal)
		{			
			
			SCR_ItemAttributeCollection collection = SCR_ItemAttributeCollection.Cast(universal.GetAttributes());
			
			if(collection)
			{
				SCR_InventoryUIInfo ui = SCR_InventoryUIInfo.Cast(collection.GetUIInfo());
				ui.SetName(string.Format("%1's Loot", name));
				ui.SetDescription(string.Format("This crate belongs to %1", description));
				
				Print(string.Format("TrainWreck: CrateId(%1) has been updated to %2", playerId, name), LogLevel.WARNING);
			}
		}
		else
			Print(string.Format("TrainWreck: LootBox RPC - unable to find SCR_UniversalInventoryStorageComponent on ID: %1", playerId), LogLevel.ERROR);		
	}
	
	protected void DelayedRpcCallToUpdate(RplId id, int playerId)
	{
		Rpc(RpcDo_UpdateCrateInfo, id, playerId);
	}
	
	void InitializeForPlayer(int playerId)
	{
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		UpdatePlayerCrateInformation(rpl.Id(), playerId);
		GetGame().GetCallqueue().CallLater(DelayedRpcCallToUpdate, SCR_TW_Util.FromSecondsToMilliseconds(5), false, rpl.Id(), playerId);
						
		this.playerId = playerId;
		GetGame().GetCallqueue().CallLater(UpdatePlayerIdServerLater, 1000, false);		
		
		if (!(rpl && rpl.Role() == RplRole.Authority))
			return;
		
		ClearInventory();
		InitializePlayerInventory();
		GetGame().GetCallqueue().CallLater(SaveAndDeleteCrate, SCR_TW_Util.FromMinutesToMilliseconds(deleteTimer), false);
	}
	
	private void UpdatePlayerIdServerLater()
	{
		Rpc(RpcDo_UpdatePlayerId, playerId);
	}
	
	private void SaveAndDeleteCrate()
	{
		SCR_TW_ExtractionHandler.GetInstance().SaveAndDeleteCrate(GetPlayerId());
	}
	
	private void ClearInventory()
	{
		InventoryStorageManagerComponent manager = TW<InventoryStorageManagerComponent>.Find(GetOwner());
		BaseInventoryStorageComponent storage = TW<BaseInventoryStorageComponent>.Find(GetOwner());
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
		
		InventoryStorageManagerComponent manager = TW<InventoryStorageManagerComponent>.Find(GetOwner());
		
		if(!manager)
		{
			Print(string.Format("TrainWreck: PlayerCrateComponent requires an InventoryStorageManagerComponent: %1", playerName), LogLevel.ERROR);
			return;
		}
		
		BaseInventoryStorageComponent storage = TW<BaseInventoryStorageComponent>.Find(GetOwner());
		
		Print("TrainWreck: Populating Player Loot Box", LogLevel.NORMAL);
		foreach(string name, int amount : items)
		{			
			Print(string.Format("TrainWreck: Inserting %1 (%2)", name, amount), LogLevel.NORMAL);
			
			Resource itemResource = Resource.Load(name);
			
			if(!itemResource.IsValid())
			{
				Print(string.Format("TrainWreck: Invalid resource for player crate: %1", name), LogLevel.ERROR);
				continue;
			}
			
			for(int i = 0; i < amount; i++)
			{			
				EntitySpawnParams params = EntitySpawnParams();
				GetOwner().GetTransform(params.Transform);
				
				IEntity item = GetGame().SpawnEntityPrefab(itemResource, GetGame().GetWorld(), params);
				
				if(!item)
					continue;
				
				BaseWeaponComponent weaponComponent = BaseWeaponComponent.Cast(item.FindComponent(BaseWeaponComponent));					
				
				if(weaponComponent)
				{
					BaseMagazineComponent magazine = weaponComponent.GetCurrentMagazine();
					if(magazine)
						SCR_EntityHelper.DeleteEntityAndChildren(magazine.GetOwner());
				}
				
				bool success = manager.TryInsertItemInStorage(item, storage);
				
				if(!success)
					SCR_EntityHelper.DeleteEntityAndChildren(item);
			}
		}
			
	}
};