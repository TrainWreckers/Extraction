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

class TW_PlayerLootItemJSON : JsonApiStruct
{
	string resourceName;
	int quantity;
	
	void TW_PlayerLootItem()
	{
		RegV("resourceName");
		RegV("quantity");
	}
}

class TW_PlayerLootJSON : JsonApiStruct
{
	string playerName;
	ref array<string> items;
	
	void TW_PlayerLootJSON()
	{
		RegV("playerName");
		RegV("items");				
	}
	
	void AddItem(string resource) 
	{ 
		if(!items)
		{
			Print("TrainWreck: Initializing json loadout");
			items = {};
		}
		items.Insert(resource); 		
	}
}

class SCR_TW_ExtractionPlayerInventoryComponent : SCR_BaseGameModeComponent
{
	protected static SCR_TW_ExtractionPlayerInventoryComponent s_Instance;
	
	// Key: player name
	// Value: saved data
	protected ref map<string, ref TW_PlayerLootJSON> m_PlayerLoadouts = new map<string, ref TW_PlayerLootJSON>();
	
	protected bool m_LocalPlayerLoadoutAvailable;
	ref SCR_PlayerLoadoutData m_LocalLoadoutData;
	
	//-----
	static SCR_TW_ExtractionPlayerInventoryComponent GetInstance()
	{
		if(!s_Instance)
			s_Instance = SCR_TW_ExtractionPlayerInventoryComponent.Cast(GetGame().GetGameMode().FindComponent(SCR_TW_ExtractionPlayerInventoryComponent));
		
		return s_Instance;
	}	
	
	bool SavePlayerLoadout(int playerId)
	{
		IEntity entity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		string name = GetGame().GetPlayerManager().GetPlayerName(playerId);
		
		SCR_CharacterInventoryStorageComponent inventory = SCR_CharacterInventoryStorageComponent.Cast(entity.FindComponent(SCR_CharacterInventoryStorageComponent));
		
		if(!inventory)
		{
			Print(string.Format("TrainWreck: %1 does not have a character inventory component", name), LogLevel.ERROR);
			return false;
		}
		
		ref array<SCR_UniversalInventoryStorageComponent> storages = {};
		inventory.GetStorages(storages);				
		
		Print(string.Format("TrainWreck: %1 has %2 storages", name, storages.Count()));
		
		ref array<InventoryItemComponent> items = {};
		ref map<string, int> loadoutMap = new map<string, int>();
		inventory.GetOwnedItems(items);
		
		// This will actually grab all equippable inventories
		// Alice, backpack, clothing, etc.
		// excludes weapons
		foreach(InventoryItemComponent item : items)
		{
			InventoryStorageSlot slot = item.GetParentSlot();
			if(!slot) continue;
			IEntity attachedEntity = slot.GetAttachedEntity();
			if(!attachedEntity) continue;
			
			ResourceName prefabName = attachedEntity.GetPrefabData().GetPrefab().GetResourceName();
			
			// at this point we have figured out the container items.
			// now we need to dive into this items to figure out what's inside them
			Print(string.Format("TrainWreck: Loadout: %1 -> %2", name, prefabName));
			
			
			BaseInventoryStorageComponent substorage = BaseInventoryStorageComponent.Cast(attachedEntity.FindComponent(BaseInventoryStorageComponent));
						
			if(!substorage)
				continue;
			ProcessInventory(name, substorage, loadoutMap);
		}
		
		BaseInventoryStorageComponent weaponStorage = inventory.GetWeaponStorage();		
		ProcessInventory(name, weaponStorage, loadoutMap, true);
		
		SCR_JsonSaveContext saveContext = new SCR_JsonSaveContext();
		
		saveContext.WriteValue("playerName", name);
		/*ref array<string> playerItems = {};
		foreach(auto itemName, auto itemCount : loadoutMap)
			for(int i = 0; i < itemCount; i++)
				playerItems.Insert(itemName);*/
		
		// Bring in existing save file
		string playerSaveFile = string.Format("%1.json", name);
		
		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		bool success = loadContext.LoadFromFile(playerSaveFile);
		if(success)
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
		
		Print(string.Format("TrainWreck: PlayerLoadout: %1 %2", name, saveContext.ExportToString()));
		saveContext.SaveToFile(playerSaveFile);
		return true;	
	}
	
	private void ProcessInventory(string name, BaseInventoryStorageComponent storage, out notnull map<string, int> loadout, bool isWeapon = false, int depth = 0)
	{
		int slotsCount = storage.GetSlotsCount();
			
		for(int i = 0; i < slotsCount; i++)
		{
			InventoryStorageSlot slot = storage.GetSlot(i);
		    if (!slot)
		       continue;
		
		    IEntity attachedEntity = slot.GetAttachedEntity();
			
			if (!attachedEntity)
				continue;
			
			/*
				Odds are if this attached entity contains storage 
				it's something similar to the alice-vest where 
				items are attached to it, but aren't actually 
				the item(s) we're looking for.
				
				So the hierarchy looks something like this
				vest > pouch > item
			
				Through recursion we go to the furthest depths
			*/
			BaseInventoryStorageComponent entityStorage = BaseInventoryStorageComponent.Cast(attachedEntity.FindComponent(BaseInventoryStorageComponent));
			
			if(entityStorage)
				ProcessInventory(name, entityStorage, loadout, depth: depth++);				
			
		    ResourceName prefabName = attachedEntity.GetPrefabData().GetPrefab().GetResourceName();		    
		
		    if (prefabName.IsEmpty())
		       continue;
				
			Print(string.Format("TrainWreck: Loadout: %1 -> %2", name, prefabName));
			
			if(!loadout.Contains(prefabName))
				loadout.Insert(prefabName, 1);
			else
				loadout.Set(prefabName, loadout.Get(prefabName) + 1);
		}
	}
	
	override void EOnInit(IEntity owner)
	{
		if(!GetGame().InPlayMode())
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
		
		string playerFilename = string.Format("%1.json", playerName);
		if(FileIO.FileExists(playerFilename))
		{
			TW_PlayerLootJSON savedData = new TW_PlayerLootJSON();
			if(savedData.LoadFromFile(playerFilename))
			{
				Print(string.Format("TrainWreck: Successfully loaded %1's loadout", playerName));
				
				if(!m_PlayerLoadouts.Contains(playerName))
					m_PlayerLoadouts.Insert(playerName, savedData);
				else
					m_PlayerLoadouts.Set(playerName, savedData);
			}
			else
			{
				Print(string.Format("TrainWreck: Failed to load %1's loadout", playerName), LogLevel.ERROR);
				return;
			}
		}
	}
};