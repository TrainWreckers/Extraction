class SCR_TW_PlayerCrateComponentClass : ScriptComponentClass {};

class SCR_TW_PlayerCrateComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.Slider, params: "0 100, 1")]
	private int playerId;
	private bool initialized = false;
	
	override void OnPostInit(IEntity owner)
	{
		if(!GetGame().InPlayMode())
			return;
		
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if (!(rpl && rpl.IsMaster() && rpl.Role() == RplRole.Authority))
			return;
		
		SCR_TW_ExtractionHandler.GetInstance().RegisterPlayerCrate(playerId, this);
	}
	
	bool CanOpen(int playerId) { return this.playerId == playerId; }
	
	void InitializeForPlayer(int playerId)
	{
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		if (!(rpl && rpl.Role() == RplRole.Authority))
			return;
		
		this.playerId = playerId;
		InitializePlayerInventory();
		//GetGame().GetCallqueue().CallLater(InitializePlayerInventory, SCR_TW_Util.FromSecondsToMilliseconds(15), false);
	}
	
	private void InitializePlayerInventory()
	{		
		if(initialized) return;
		initialized = true;
		
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
				if(!success)
					Print(string.Format("TrainWreck: Failed to insert %1 for %2", name, playerName), LogLevel.ERROR);
			}
		}
			
	}
};