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
	//! RPC Call to server to ensure only the server updates/saves inventory 
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
		SCR_InventoryStorageManagerComponent manager = TW<SCR_InventoryStorageManagerComponent>.Find(entity);
		
		ref array<IEntity> allItems = {};
		int count = manager.GetItems(allItems);
		
		Print(string.Format("TrainWreck: %1 has %2 items", name, count), LogLevel.WARNING);
		ref map<string, int> loadoutMap = new map<string, int>();
		
		PrintFormat("TrainWreck: Global Catalog is tracking %1 items", SCR_TW_ExtractionHandler.GetInstance().GetCatalogCount());
		
		foreach(IEntity item : allItems)
		{
			ResourceName resource = item.GetPrefabData().GetPrefab().GetResourceName();
			
			if(!SCR_TW_ExtractionHandler.GetInstance().IsValidItem(resource))
			{
				PrintFormat("TrainWreck: %1 is not a valid item for saving", resource);
				continue;
			}
			
			if(loadoutMap.Contains(resource))
				loadoutMap.Set(resource, loadoutMap.Get(resource) + 1);
			else
				loadoutMap.Insert(resource, 1);
		}
		
		string filename = string.Format("$profile:%1.json", name);
		
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
		
		
		SCR_JsonSaveContext saveContext = new SCR_JsonSaveContext();
		saveContext.WriteValue("playerName", name);
		saveContext.WriteValue("items", loadoutMap);
		bool success = saveContext.SaveToFile(filename);
		return success;		
	}
	
	override void EOnInit(IEntity owner)
	{
		if(!TW_Global.IsInRuntime())
			return;
		
		if(!TW_Global.IsServer(GetOwner()))
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