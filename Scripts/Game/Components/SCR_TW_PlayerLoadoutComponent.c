class SCR_TW_PlayerLoadoutComponentClass : ScriptComponentClass {};

class SCR_TW_PlayerLoadoutComponent : ScriptComponent
{
	private InventoryStorageManagerComponent storageManager;
	
	void InitializePlayerInventory(string jsonLoadout)
	{
		storageManager = InventoryStorageManagerComponent.Cast(GetOwner().FindComponent(InventoryStorageManagerComponent));
		
		if(!storageManager)
		{
			Debug.Error("TrainWreck: Player does not have an inventory storage manager component");
			return;
		}
		
		
		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		bool success = loadContext.ImportFromString(jsonLoadout);
		
		if(!success)
		{
			Debug.Error(string.Format("TrainWreck: Failed to parse %1", jsonLoadout));
			return;
		}		
	}
		
};