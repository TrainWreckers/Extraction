[EntityEditorProps(category: "GameScripted/TrainWreck/Systems", description: "Handles extraction related setup at a game-mode level")]
class SCR_TW_ExtractionHandlerClass : SCR_BaseGameModeComponentClass {};


class SCR_TW_ExtractionHandler : SCR_BaseGameModeComponent
{
	static SCR_TW_ExtractionHandler s_Instance;
	
	static SCR_TW_ExtractionHandler GetInstance()
	{
		BaseGameMode gameMode = GetGame().GetGameMode();
		
		if(!gameMode)
			return null;
		
		if(!s_Instance)
			s_Instance = SCR_TW_ExtractionHandler.Cast(gameMode.FindComponent(SCR_TW_ExtractionHandler));
		
		return s_Instance;
	}
	
	private ref map<SCR_EArsenalItemType, ref array<SCR_ArsenalItem>> lootMap = new map<SCR_EArsenalItemType, ref array<SCR_ArsenalItem>>();
	private ref map<SCR_EArsenalItemType, ref array<ref TW_LootConfigItem>> lootTable = new ref map<SCR_EArsenalItemType, ref array<ref TW_LootConfigItem>>();	
	private ref map<int, SCR_TW_PlayerCrateComponent> crates = new map<int, SCR_TW_PlayerCrateComponent>();
	private ref array<SCR_SiteSlotEntity> possibleSpawnAreas = {};
	private ref array<SCR_TW_ExtractionSite> possibleExtractionSites = {};
	private ref array<SCR_EArsenalItemType> arsenalItemTypes = {};
	private bool playersHaveSpawned = false;
	
	protected bool m_MatchOver;
	protected EGameOverTypes m_GameOverType = EGameOverTypes.NEUTRAL;
	
	private ref set<string> globalItems = new set<string>();
	
	bool IsValidItem(ResourceName resource)
	{
		return globalItems.Contains(resource);
	}
	
	[Attribute("{CB7CDB3864826FD3}Prefabs/Props/Military/AmmoBoxes/EquipmentBoxStack/TW_PlayerLoadoutCrate.et", UIWidgets.ResourcePickerThumbnail, category: "Player Spawn", desc: "Player loadout crate prefab", params: "et")]
	private ResourceName playerCratePrefab;
		
	[Attribute("{5A52168A894DDB7E}Prefabs/Compositions/Slotted/SlotFlatSmall/TW_US_PlayerHub_Extraction.et", UIWidgets.ResourcePickerThumbnail, params: "et", category: "Player Spawn", desc: "Composition to spawn as a player starting area")]
	private ResourceName playerHubPrefab;
	
	[Attribute("", UIWidgets.Slider, params: "3, 20, 1", category: "Player Spawn", desc: "After this timer elapses, the player spawn composition is deleted")]
	private int playerHubDespawnTimerInMinutes;
	
	[Attribute("3", UIWidgets.Slider, params: "1, 5, 1", category: "Extraction", desc: "Number of potential extractions available at once")]
	private int numberOfExtractionSites;	
	
	[Attribute("60", UIWidgets.Slider, params: "5 180 5", category: "Extraction", desc: "Time in minutes until Game Mode ends")]
	private int gameModeDurationInMinutes;
	
	[Attribute("30", UIWidgets.Slider, params: "5, 500, 5", category: "Extraction", desc: "Time in seconds players must wait in extraction until they get extracted")]
	private int extractionTimePeriod;
	
	[Attribute("3", UIWidgets.Slider, params: "1 50 1", category: "Insertion", desc: "Maximum number of insertion points that may appear")]
	private int numberOfInsertionPoints;
	
	[Attribute("0.25", UIWidgets.Slider, params: "0.01 1 0.01", category: "Loot Ammo Spawn", desc: "Minimum percentage of ammo per weapon/mag")]
	private float minimumAmmoPercent;
	
	[Attribute("1", UIWidgets.Slider, params: "0.01 1 0.01", category: "Loot Ammo Spawn", desc: "Maximum percentage of ammo per weapon/mag")]
	private float maximumAmmoPercent;
	
	float GetRandomAmmoPercent()
	{
		return Math.RandomFloat(minimumAmmoPercent, maximumAmmoPercent);
	}
	
	int GetExtractionTimePeriod() { return extractionTimePeriod; }
	
	void RegisterExtractionSite(SCR_TW_ExtractionSite site)
	{
		if(!possibleExtractionSites.Contains(site))
			possibleExtractionSites.Insert(site);				
	}
	
	void RegisterPlayerLoadoutCrate(SCR_TW_PlayerCrateComponent crate)
	{
		int playerId = crate.GetPlayerId();
		
		if(playerId < 0)
			return;
		
		if(crates.Contains(playerId))
		{
			Print(string.Format("TrainWreck: Crate for %1 has already been registered. Removing already registered crate", playerId), LogLevel.WARNING);
			SaveAndDeleteCrate(playerId);
			crates.Set(playerId, crate);
		}
		else
			crates.Insert(playerId, crate);
	}
	
	void UnregisterPlayerLoadoutCrate(SCR_TW_PlayerCrateComponent crate, int playerId)
	{
		if(!crates.Contains(playerId))
			return;
		
		crates.Remove(playerId);
	}
	
	private void DeleteCrateLater(IEntity owner)
	{
		SCR_EntityHelper.DeleteEntityAndChildren(owner);
	}
	
	void RegisterSpawnArea(SCR_SiteSlotEntity spawnSlot)
	{
		possibleSpawnAreas.Insert(spawnSlot);
	}
	
	override void OnPlayerKilled(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
	{
		super.OnPlayerKilled(playerId, playerEntity, killerEntity, killer);
		
		if(!crates.Contains(playerId))
			return;
		
		SaveAndDeleteCrate(playerId);
	}
	
	override void OnPlayerSpawned(int playerId, IEntity controlledEntity)
	{
		super.OnPlayerSpawned(playerId, controlledEntity);
		playersHaveSpawned = true;
		
		vector forwardDirection = SCR_TW_Util.GetForwardVec(controlledEntity) * (playerId * 3);
		vector position = controlledEntity.GetOrigin() + forwardDirection;
		
		Resource crateResource = Resource.Load(playerCratePrefab);
		
		if(!crateResource.IsValid())
		{
			Debug.Error("TrainWreck: Invalid crate prefab");
			return;
		}
		
		EntitySpawnParams params = EntitySpawnParams();
		controlledEntity.GetTransform(params.Transform);				
		
		position[1] = GetGame().GetWorld().GetSurfaceY(position[0], position[2]);
		params.Transform[3] = position;
		
		IEntity crateEntity = GetGame().SpawnEntityPrefab(crateResource, GetGame().GetWorld(), params);
		
		if(!crateEntity)
		{
			Debug.Error("TrainWreck: Crate couldn't spawn");
			return;
		}
		
		SCR_TW_PlayerCrateComponent crate = SCR_TW_PlayerCrateComponent.Cast(crateEntity.FindComponent(SCR_TW_PlayerCrateComponent));
		
		if(!crate)
		{
			Debug.Error("TrainWreck: Could not locate SCR_TW_PlayerCrateComponent");
			return;
		}
		
		crate.InitializeForPlayer(playerId);
	}
	
	//------------------------------------------------------------------------------------------------
	//! RPC Call to server to ensure only the server udpates/saves inventory 
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcUpdatePlayerCrate(int playerId)
	{
		if(!crates.Contains(playerId))
			return;
		
		crates.Get(playerId).InitializeForPlayer(playerId);
	}
	
	void SaveAndDeleteCrate(int playerId)
	{
		if(!crates.Contains(playerId))
			return;
		
		auto crate = crates.Get(playerId);
		crate.SaveCrateContents_S();
		crates.Remove(playerId);
		
		SCR_EntityHelper.DeleteEntityAndChildren(crate.GetOwner());		
	}
	
	//------------------------------------------------------------------------------------------------
	
	void UpdatePlayerInventoryCrate(int playerId)
	{
		Rpc(RpcUpdatePlayerCrate, playerId);
	}
	
	private bool OutputLootTableFile()
	{
		SCR_JsonSaveContext saveContext = new SCR_JsonSaveContext();
		
		foreach(SCR_EArsenalItemType type, ref array<SCR_ArsenalItem> items : lootMap)
		{
			ref array<ref TW_LootConfigItem> typeLoot = {};
			
			foreach(SCR_ArsenalItem item : items)
			{
				ref TW_LootConfigItem lootItem = new TW_LootConfigItem();
				ResourceName resourceName = item.GetItemResourceName();
				float chance = item.GetItemChanceToSpawn();
				int count = item.GetItemMaxSpawnCount();
				lootItem.SetData(resourceName, chance, count);
				
				typeLoot.Insert(lootItem);
			}
			
			saveContext.WriteValue(SCR_TW_Util.ArsenalTypeAsString(type), typeLoot);
		}
		
		bool success = saveContext.SaveToFile("$profile:lootmap.json");
		
		return success;
	}
	
	private bool HasLootTable()
	{
		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		return loadContext.LoadFromFile("$profile:lootmap.json");
	}
	
	private bool IngestLootTableFromFile()
	{
		SCR_JsonLoadContext loadContext = new SCR_JsonLoadContext();
		bool loadSuccess = loadContext.LoadFromFile("$profile:lootmap.json");
		
		if(!loadSuccess)
		{
			Print("TrainWreck: Was unable to load loot map. Please verify it exists, and has valid syntax");
			return false;
		}
		
		array<SCR_EArsenalItemType> itemTypes = {};
		SCR_Enum.GetEnumValues(SCR_EArsenalItemType, itemTypes);		
		string name = string.Empty;
		
		foreach(SCR_EArsenalItemType itemType : itemTypes)
		{
			if(!LoadSection(loadContext, itemType))
			{
				name = SCR_Enum.GetEnumName(SCR_EArsenalItemType, itemType);
				Print(string.Format("TrainWreck: LootMap: Unable to load %1", name), LogLevel.ERROR);
			}
		}
				
		return true;
	}
	
	private bool LoadSection(notnull SCR_JsonLoadContext context, SCR_EArsenalItemType type)
	{		
		array<ref TW_LootConfigItem> items = {};
		string keyValue = SCR_TW_Util.ArsenalTypeAsString(type);
		
		bool success = context.ReadValue(keyValue, items);
		
		if(success)
			lootTable.Insert(type, items);
		else
			lootTable.Insert(type, {});
		
		foreach(TW_LootConfigItem item : items)
			if(!globalItems.Contains(item.resourceName))
				globalItems.Insert(item.resourceName);
		
		return success;
	}
	
	override void OnGameModeStart()
	{
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		
		if(!rpl.IsMaster() && rpl.Role() != RplRole.Authority)
			return;
		
		SCR_Enum.GetEnumValues(SCR_EArsenalItemType, arsenalItemTypes);
		
		GetGame().GetCallqueue().CallLater(InitializePlayerHub, SCR_TW_Util.FromSecondsToMilliseconds(1), false);
		GetGame().GetCallqueue().CallLater(InitializeExtractionSites, SCR_TW_Util.FromSecondsToMilliseconds(1), false);
		GetGame().GetCallqueue().CallLater(CheckPlayerWipe, SCR_TW_Util.FromSecondsToMilliseconds(15), true);
		
		InitializeLootMap();
		
		int globalCount = SCR_TW_InventoryLoot.GlobalLootContainers.Count();
		
		Print(string.Format("TrainWreck: Loot Containers -> %1", globalCount), LogLevel.NORMAL);
		
		int validCount = 0;
		foreach(auto container : SCR_TW_InventoryLoot.GlobalLootContainers)
			if(container)
				validCount++;
		
		Print(string.Format("TrainWreck: Valid(%1) Invalid(%2)", validCount, globalCount-validCount), LogLevel.WARNING);
		
		foreach(SCR_TW_InventoryLoot container : SCR_TW_InventoryLoot.GlobalLootContainers)
		{					
			if(!container) 
				continue;
			
			string format = string.Format("TrainWreck: %1", container.GetOwner().GetPrefabData().GetPrefabName());
			int spawnCount = Math.RandomIntInclusive(1, 6);
			
			// How many different things are we going to try spawning?								
			for(int i = 0; i < spawnCount; i++)
			{	
				auto arsenalItem = GetRandomItemByFlag(container.GetTypeFlags());
			
				if(!arsenalItem)
					break;
				
				// Are we going to spawn the selected item?
				float seedPercentage = Math.RandomFloat(0.001, 100);
				if(arsenalItem.chanceToSpawn > seedPercentage)
					continue;
				
				// Add item a random amount of times to the container based on settings
				int itemCount = Math.RandomIntInclusive(1, arsenalItem.randomSpawnCount);
				bool tryAgain = false;
				for(int x = 0; x < itemCount; x++)
				{
					bool success = container.InsertItem(arsenalItem);
					
					if(!success)
					{
						tryAgain = true;
						break;
					}
				}
				
				if(tryAgain)
					spawnCount--;
			}
		}
	}
	
	private void InitializeExtractionSites()
	{
		if(possibleExtractionSites.IsEmpty())
			return;
		
		int randomCount = Math.RandomIntInclusive(1, numberOfExtractionSites);
		randomCount = Math.Min(randomCount, possibleExtractionSites.Count());
		
		int exclude = possibleExtractionSites.Count() - randomCount;
		for(int i = 0; i < exclude; i++)
		{
			int index = possibleExtractionSites.GetRandomIndex();
			SCR_TW_ExtractionSite site = possibleExtractionSites.Get(index);
			SCR_EntityHelper.DeleteEntityAndChildren(site);
			possibleExtractionSites.Remove(index);
		}
			
		foreach(SCR_TW_ExtractionSite site : possibleExtractionSites)
			site.SpawnSite();
	}
	
	private void CheckPlayerWipe()
	{
		// Players must have spawned in at least 1 time for things to be considered a wipe.
		// Otherwise a fresh server may end up restarting infinitely while no one is on 
		
		if(!playersHaveSpawned && !m_MatchOver)
			return;
		
		ref array<int> playerIds = {};
		int playerCount = GetGame().GetPlayerManager().GetPlayers(playerIds);
		
		if(playerCount <= 0)
		{
			m_GameOverType = EGameOverTypes.SERVER_RESTART;
			FinishGame();
		}			
	}
	
	private void InitializePlayerHub()
	{
		if(possibleSpawnAreas.IsEmpty())
		{
			Print("TrainWreck: No spawn points have been registered", LogLevel.ERROR);
			return;
		}
		
		int randomCount = Math.RandomInt(1, Math.Min(numberOfInsertionPoints, possibleSpawnAreas.Count()));
		
		for(int i = 0; i < randomCount; i++)
		{
			int index = possibleSpawnAreas.GetRandomIndex();
			SCR_SiteSlotEntity site = possibleSpawnAreas.Get(index);
			possibleSpawnAreas.Remove(index);
			
			Resource hubResource = Resource.Load(playerHubPrefab);
			if(!hubResource.IsValid())
			{
				Debug.Error(string.Format("TrainWreck: Unable to spawn %1, invalid resource", possibleSpawnAreas));
				return;
			}
			
			IEntity spawnSiteEntity = site.SpawnEntityInSlot(hubResource);
			
			if(!spawnSiteEntity)
			{
				Print(string.Format("TrainWreck: Was unable to spawn player hub. %1", playerHubPrefab), LogLevel.ERROR);
				return;
			}
		}		
	}
	
	private void DespawnPlayerSpawn(IEntity entity)
	{
		SCR_EntityHelper.DeleteEntityAndChildren(entity);
	}
	
	private void InitializeLootMap()
	{
		if(HasLootTable())
		{
			Print("TrainWreck: lootmap.json file discovered");
			IngestLootTableFromFile();
			return;
		}
		
		ref array<Faction> allFactions = {};
		GetGame().GetFactionManager().GetFactionsList(allFactions);
		
		SCR_EntityCatalog globalCatalog;
		
		foreach(auto currentFaction : allFactions)
		{
			auto faction = SCR_Faction.Cast(currentFaction);
			
			if(!faction)
				continue;
			
			auto factionCatalog = faction.GetFactionEntityCatalogOfType(EEntityCatalogType.ITEM);
			
			if(!globalCatalog)
				globalCatalog = factionCatalog;
			else
				globalCatalog.MergeCatalogs(factionCatalog);				
		}
		
		// Process the global catalog of items now
		ref array<SCR_EntityCatalogEntry> catalogItems = {};
		int entityCount = globalCatalog.GetEntityList(catalogItems);
		
		foreach(auto entry : catalogItems)
		{
			ref array<SCR_BaseEntityCatalogData> itemData = {};
			entry.GetEntityDataList(itemData);
			
			// We only care about fetching Arsenal Items
			foreach(auto data : itemData)
			{
				auto arsenalItem = SCR_ArsenalItem.Cast(data);
				
				if(!arsenalItem)	
					continue;
				
				if(!arsenalItem.IsEnabled())
					break;
				
				auto itemType = arsenalItem.GetItemType();
				auto itemMode = arsenalItem.GetItemMode();
				auto prefab = entry.GetPrefab();
				
				arsenalItem.SetItemPrefab(prefab);
				
				if(!arsenalItem)
				{
					Print("TrainWreck: Failed to create LootItem during initialization");
					break;	
				}
				
				if(lootMap.Contains(itemType))
					lootMap.Get(itemType).Insert(arsenalItem);
				else
				{
					lootMap.Insert(itemType, {});					
					lootMap.Get(itemType).Insert(arsenalItem);
				}				
			}
		}
		
		// Finally, we can write to json file!
		bool success = OutputLootTableFile();		
		if(!success)
			Print("TrainWreck: Failed to write lootmap.json", LogLevel.ERROR);
	}
	
	TW_LootConfigItem GetRandomItemByFlag(int type)
	{
		array<SCR_EArsenalItemType> selectedItems = {};
		
		if(type > 0)
		{
			foreach(SCR_EArsenalItemType itemType : arsenalItemTypes)
			{
				if(SCR_Enum.HasFlag(type, itemType) && lootTable.Contains(itemType))
					selectedItems.Insert(itemType);
			}			
		}
		else 
			return null;
		
		/*
		else
		{
			foreach(SCR_EArsenalItemType itemType : arsenalItemTypes)
			{
				if(lootTable.Contains(itemType))
					selectedItems.Insert(itemType);
			}
		}
		*/
		
		// Check if nothing was selected 
		if(selectedItems.IsEmpty())
			return null;
		
		auto items = lootTable.Get(selectedItems.GetRandomElement());
		
		// Check if nothing was available 
		if(!items || items.IsEmpty())
			return null;
		
		return items.GetRandomElement();
	}		
	
	bool IsMaster() // IsServer 
	{
		RplComponent rpl = RplComponent.Cast(GetOwner().FindComponent(RplComponent));
		
		if(!rpl)
			return false;
		
		return !rpl.IsProxy();
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_PlaySoundOnEntity(EntityID entityID, string soundName)
	{
		if(!entityID)
			return;
		
		IEntity entity = GetGame().GetWorld().FindEntityByID(entityID);
		
		if(!entity)
			return;
		
		SCR_CommunicationSoundComponent soundComp = SCR_CommunicationSoundComponent.Cast(entity.FindComponent(SCR_CommunicationSoundComponent));
		
		if(!soundComp)
			return;
		
		soundComp.PlayStr(soundName);
	}
	
	void PlaySoundOnEntity(IEntity entity, string soundName)
	{
		if(!entity)
			entity = GetOwner();
		
		if(!entity)
			return;
		
		if(IsMaster())
			Rpc(RpcDo_PlaySoundOnEntity, entity.GetID(), soundName);
		RpcDo_PlaySoundOnEntity(entity.GetID(), soundName);
	}
	
	void PlaySoundOnPlayer(string soundName)
	{
		SCR_PlayerController pc = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		
		if(!pc)
			return;
		
		IEntity player = pc.GetMainEntity();
		if(!player)
			return;
		
		PlaySoundOnEntity(player, soundName);
	}
	
	void SetMissionEndScreen(EGameOverTypes gameOverType)
	{
		m_GameOverType = gameOverType;
	}
	
	void EndGame()
	{
		SCR_GameModeEndData endData = SCR_GameModeEndData.CreateSimple(m_GameOverType, 0, 0);
		m_MatchOver = true;
		SCR_BaseGameMode.Cast(GetOwner()).EndGameMode(endData);
	}
	
	bool GetIsMatchOver()
	{
		return m_MatchOver;
	}
	
	void PopUpMessage(string title, string subtitle)
	{
		// Ensure this gets broadcasted to all players on server
		Rpc(RpcDo_PopUpMessage, title, subtitle);
		
		SCR_PopUpNotification.GetInstance().PopupMsg(title, text2: subtitle);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_PopUpMessage(string title, string subtitle)
	{
		SCR_PopUpNotification.GetInstance().PopupMsg(title, text2: subtitle);
	}
	
	void FinishGame()
	{
		SCR_GameModeEndData endData = SCR_GameModeEndData.CreateSimple(m_GameOverType, 0,0);
		
		m_MatchOver = true;
		
		SCR_BaseGameMode.Cast(GetOwner()).EndGameMode(endData);
	}
};