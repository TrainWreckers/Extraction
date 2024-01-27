class SCR_TW_PlayerCrateComponentClass : ScriptComponentClass {};

class SCR_TW_PlayerCrateComponent : ScriptComponent
{
	[RplProp(onRplName: "onPlayerNameChange")]
	private string playerName;
	
	[Attribute("5", UIWidgets.Slider, params: "1 120 5", desc: "Timer (in minutes), crate will be deleted")]
	private int deleteTimer;
	
	bool CanOpen(string name) { return playerName == name; }
	
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
				
	}
	
	void onPlayerNameChange()
	{
		string name = string.Format("%1's Crate", playerName);
		string description = string.Format("This crate belongs to %1", playerName);
		
		SCR_TW_PlayerCrateComponent crate = TW<SCR_TW_PlayerCrateComponent>.Find(GetOwner());
		
		if(!crate)
		{
			Print(string.Format("TrainWreck: LootBox RPC - unable to find SCR_TW_PlayerCrateComponent on ID %1", playerName), LogLevel.ERROR);			
			return;
		}
		
		SCR_UniversalInventoryStorageComponent universal = TW<SCR_UniversalInventoryStorageComponent>.Find(crate.GetOwner());
		if(universal)
		{			
			
			SCR_ItemAttributeCollection collection = SCR_ItemAttributeCollection.Cast(universal.GetAttributes());
			
			if(collection)
			{
				SCR_InventoryUIInfo ui = SCR_InventoryUIInfo.Cast(collection.GetUIInfo());
				ui.SetName(name);
				ui.SetDescription(description);
				
				Print(string.Format("TrainWreck: CrateId(%1)", name), LogLevel.WARNING);
			}
		}
		else
			Print(string.Format("TrainWreck: LootBox RPC - unable to find SCR_UniversalInventoryStorageComponent: %1", playerName), LogLevel.ERROR);		
	}
	
	//------------------------------------------
	// Called by server
	void InitializeForPlayer(int newPlayerId)
	{
		if(!TW_Global.IsServer(GetOwner()))
			return;
		
		playerName = GetGame().GetPlayerManager().GetPlayerName(newPlayerId);
		//Replication.BumpMe();
		
		onPlayerNameChange();
			
		ClearInventory();
		InitializePlayerInventory();
		GetGame().GetCallqueue().CallLater(SaveAndDeleteCrate, SCR_TW_Util.FromMinutesToMilliseconds(deleteTimer), false);
	}
	
	private void SaveAndDeleteCrate()
	{
		ref array<int> playerIds = {};		
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		
		int useId = 0;
		foreach(int playerId : playerIds)
		{
			if(playerName == GetGame().GetPlayerManager().GetPlayerName(playerId))
			{
				useId = playerId;
				break;
			}
		}
		
		SCR_TW_ExtractionHandler.GetInstance().SaveAndDeleteCrate(useId);
	}
	
	private void ClearInventory()
	{
		InventoryStorageManagerComponent manager = TW<InventoryStorageManagerComponent>.Find(GetOwner());
		BaseInventoryStorageComponent storage = TW<BaseInventoryStorageComponent>.Find(GetOwner());
		ref array<IEntity> items = {};
		manager.GetItems(items);
		
		foreach(IEntity item : items)
			SCR_EntityHelper.DeleteEntityAndChildren(item);
	}
	
	// --------------------------------------
	// Should be called via server 
	private void InitializePlayerInventory()
	{		
		if(playerName == string.Empty)
			return;
		
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