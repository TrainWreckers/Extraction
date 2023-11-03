class SCR_TW_PlayerCrateComponentClass : ScriptComponentClass {};

class SCR_TW_PlayerCrateComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.Slider, params: "0 100, 1")]
	private int playerId;
	
	override void OnPostInit(IEntity owner)
	{
		if(!GetGame().InPlayMode())
			return;
		
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if(!rpl) return;
		if(!rpl.IsMaster()) return;
		
		SCR_TW_ExtractionHandler.GetInstance().RegisterPlayerCrate(playerId, this);		
		GetGame().GetCallqueue().CallLater(InitializePlayerInventory, SCR_TW_Util.FromSecondsToMilliseconds(15), false);
	}
	
	bool CanOpen(int playerId) { return this.playerId == playerId; }
	
	void InitializeForPlayer(int playerId)
	{
		this.playerId = playerId;
		GetGame().GetCallqueue().CallLater(InitializePlayerInventory, SCR_TW_Util.FromSecondsToMilliseconds(15), false);
	}
	
	private void InitializePlayerInventory()
	{
		string playerName = GetGame().GetPlayerManager().GetPlayerName(playerId);
		string filename = string.Format("%1.json", playerName);
		
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
		
		foreach(string name, int amount : items)
		{
			string text = string.Format("TrainWreck: %1 -> Attempting to spawn %2", playerId, name);
			
			for(int i = 0; i < amount; i++)
			{
				bool success = manager.TrySpawnPrefabToStorage(name);
				if(success)
					text = string.Format("%1 - SUCCESS - (%2/%3)", text, i+1, amount);
				else
					text = string.Format("%1 - FAIL - (%2/%3)", text, i+1, amount);
				Print(text);
			}
		}
			
	}
};