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
	
	private ref array<TW_RadioTower_Component> radioTowers = {};
	private ref map<int, SCR_TW_PlayerCrateComponent> crates = new map<int, SCR_TW_PlayerCrateComponent>();
	private ref array<SCR_SiteSlotEntity> possibleSpawnAreas = {};
	private ref array<SCR_TW_ExtractionSite> possibleExtractionSites = {};		
	private bool playersHaveSpawned = false;
	
	protected bool m_MatchOver;
	protected EGameOverTypes m_GameOverType = EGameOverTypes.NEUTRAL;
	
	private ref set<string> globalItems = new set<string>();	
	
	void RegisterRadioTower(TW_RadioTower_Component tower)
	{
		if(!radioTowers.Contains(tower))
			radioTowers.Insert(tower);
	}
	
	void UnregisterRadioTower(TW_RadioTower_Component tower)
	{
		if(radioTowers.Contains(tower))
			radioTowers.RemoveItem(tower);
	}
	
	TW_RadioTower_Component GetRandomTower()
	{
		return radioTowers.GetRandomElement();
	}
	
	bool IsValidItem(ResourceName resource)
	{
		return TW_LootManager.IsValidItem(resource) || resource == "{6D56FED1E55A8F84}Prefabs/Items/Misc/IntelligenceFolder_E_01/IntelligenceFolder_E_01.et";
	}
	
	int GetCatalogCount() { return globalItems.Count(); }
	
	[Attribute("0", UIWidgets.CheckBox, category: "Systems", desc: "Larger the map, longer it takes to start scenario. May want to disable during rapid testing of a non-loot item")]
	private bool m_EnableLootSpawns;
	
	[Attribute("{FF9846A7C3FFC487}Prefabs/Props/Military/AmmoBoxes/EquipmentBoxStack/TW_PlayerLoadoutCrateInvisible.et", UIWidgets.ResourcePickerThumbnail, category: "Player Spawn", desc: "Invisible crate used for managing player loot", params: "et")]
	private ResourceName m_InvisibleLootCratePrefab;
	
	[Attribute("{5A52168A894DDB7E}Prefabs/Compositions/Slotted/SlotFlatSmall/TW_US_PlayerHub_Extraction.et", UIWidgets.ResourcePickerThumbnail, params: "et", category: "Player Spawn", desc: "Composition to spawn as a player starting area")]
	private ResourceName playerHubPrefab;
	
	[Attribute("", UIWidgets.Auto, params: "conf SCR_EntityCatalogMultiList", desc: "Catalogs of items that are lootable")]
	private ref array<ref SCR_EntityCatalogMultiList> catalogConfigs;
	
	[Attribute("60", UIWidgets.Slider, params: "5 180 5", category: "Extraction", desc: "Time in minutes until Game Mode ends")]
	private int gameModeDurationInMinutes;
	
	[Attribute("30", UIWidgets.Slider, params: "5, 500, 5", category: "Extraction", desc: "Time in seconds players must wait in extraction until they get extracted")]
	private int extractionTimePeriod;
	
	[Attribute("5", UIWidgets.Slider, params: "1 50 1", category: "Insertion", desc: "Maximum number of insertion points that may appear")]
	private int numberOfInsertionPoints;
	
	[Attribute("0.25", UIWidgets.Slider, params: "0.01 1 0.01", category: "Loot Ammo Spawn", desc: "Minimum percentage of ammo per weapon/mag")]
	private float minimumAmmoPercent;
	
	[Attribute("1", UIWidgets.Slider, params: "0.01 1 0.01", category: "Loot Ammo Spawn", desc: "Maximum percentage of ammo per weapon/mag")]
	private float maximumAmmoPercent;
	
	[Attribute("0.5", UIWidgets.Slider, params: "0.01 1 0.01", category: "Loot Ammo Spawn", desc: "Chance a magazine won't spawn with a gun")]
	private float chanceOfMagazine;
	
	[Attribute("", UIWidgets.ResourceNamePicker, params: "et", category: "Extraction", desc: "Task prefab to inform players where extraction points are")]
	private ResourceName m_ExtractionTaskPrefab;
	
	[Attribute("", UIWidgets.EditBox, category: "Extraction", desc: "Text format to use for extraction description")]
	private string m_ExtractionTaskDescriptionFormat;
	
	
	ResourceName GetExtractionTaskPrefab() { return m_ExtractionTaskPrefab; }
	
	//! Prefab which has all container related components needed to perform storage operations
	ResourceName GetPlaceholderCratePrefab() { return m_InvisibleLootCratePrefab; }
	
	string GetExtractionDescription(vector position)
	{
		if(m_ExtractionTaskDescriptionFormat == string.Empty)
			return string.Empty;
		
		int x = (int)(position[0] / 1000);
		int y = (int)(position[2] / 1000);
		
		return string.Format(m_ExtractionTaskDescriptionFormat, x, y);
	}
	
	float ShouldSpawnMagazine()
	{
		return Math.RandomFloat(0, 1) <= chanceOfMagazine;
	}
	
	float GetRandomAmmoPercent()
	{
		return Math.RandomFloat(minimumAmmoPercent, maximumAmmoPercent);
	}
	
	array<ref SCR_EntityCatalogMultiList> GetCatalogConfigs() { return catalogConfigs; }
	
	int GetExtractionTimePeriod() { return extractionTimePeriod; }
	
	//! Globally register extraction site for usage
	void RegisterExtractionSite(SCR_TW_ExtractionSite site)
	{
		if(!possibleExtractionSites.Contains(site))
			possibleExtractionSites.Insert(site);				
	}
	
	//! Is provided site valid for use
	private bool IsValidExtractionSite(SCR_TW_ExtractionSite site, bool isEmpty)
	{
		if(site == null)
			return false;
		
		if(isEmpty && site.GetOccupant() != null)
			return false;
		
		return true;
	}
	
	//! Grab an extraction site from pool of registered sites.
	SCR_TW_ExtractionSite GetRegisteredExtractionSite(bool isEmpty = true)
	{
		ref array<SCR_TW_ExtractionSite> sites = {};
		sites.Copy(possibleExtractionSites);		
		
		SCR_TW_ExtractionSite site;
				
		while(IsValidExtractionSite(site, isEmpty))
		{
			int index = Math.RandomInt(0, sites.Count());
			site = sites.Get(index);
			sites.Remove(index);
		}
		
		return site;
	}
	
	//! Attempts to get a random extraction site that is not currently in use
	SCR_TW_ExtractionSite GetRandomExtractionSite()
	{
		ref array<SCR_TW_ExtractionSite> queue = {};
		queue.Copy(possibleExtractionSites);
		
		int index = queue.GetRandomIndex();		
		SCR_TW_ExtractionSite site = queue.Get(index);
		
		while(site != null && site.GetOccupant() != null)
		{
			queue.Remove(index);
			
			if(queue.IsEmpty())
				return null;
			
			index = queue.GetRandomIndex();
			site = queue.Get(index);
		}
		
		return site;
	}
	
	void RegisterSpawnArea(SCR_SiteSlotEntity spawnSlot)
	{
		possibleSpawnAreas.Insert(spawnSlot);
	}
	
	override void OnPlayerKilled(int playerId, IEntity playerEntity, IEntity killerEntity, notnull Instigator killer)
	{
		super.OnPlayerKilled(playerId, playerEntity, killerEntity, killer);
		
		if(!Replication.IsServer())
			return;
		
		if(!crates.Contains(playerId))
			return;
		
		SaveAndDeleteCrate(playerId);
		
		if(TW_MissionDownload.IsDownloadActive())
		{
			// If the player is inside this mission
			// it will remove them 
			TW_MissionDownload.GetActiveMission().RemovePlayer(playerId);
		}
	}
	
	//------------------------------------------------------------------------------------------------
	override void OnPlayerSpawnFinalize_S(SCR_SpawnRequestComponent requestComponent, SCR_SpawnHandlerComponent handlerComponent, SCR_SpawnData data, IEntity entity)
	{
		super.OnPlayerSpawnFinalize_S(requestComponent, handlerComponent, data, entity);
		
		if(!TW_Global.IsServer(GetOwner()))
			return;
		
		playersHaveSpawned = true;
		
		if(!SCR_TW_Util.PlayerHasLootFile(requestComponent.GetPlayerId()))
		{
			Print(string.Format("TrainWreck: Player does not have a loot file. Ignoring showing loot menu: %1", requestComponent.GetPlayerId()));
			return;
		}
		
		SCR_PlayerController controller = SCR_PlayerController.Cast(requestComponent.GetPlayerController());
		if(!controller)
		{
			Print(string.Format("TrainWreck: Invalid player controller for opening loot menu: %1", requestComponent.GetPlayerId()), LogLevel.ERROR);
			return;
		}
		
		SCR_TW_PlayerCrateComponent playerCrate = SpawnPlayerCrate(requestComponent, entity);
		
		if(!playerCrate)
		{
			Print(string.Format("TrainWreck: Failed to spawn loot crate for player %1", requestComponent.GetPlayerId()), LogLevel.ERROR);
			return;
		}
		
		RplComponent rplComp = TW<RplComponent>.Find(playerCrate.GetOwner());
		RplId rplId = rplComp.Id();

		controller.OnPlayerSpawnedEvent(rplId);
	}
	
	private SCR_TW_PlayerCrateComponent SpawnPlayerCrate(SCR_SpawnRequestComponent requestComponent, IEntity playerEntity)
	{
		int playerId = requestComponent.GetPlayerId();
		
		// Lets spawn the player crate at game mode position
		EntitySpawnParams crateSpawnParams = EntitySpawnParams();
		playerEntity.GetTransform(crateSpawnParams.Transform);
		
		Resource crateResource = Resource.Load(GetPlaceholderCratePrefab());
		if(!crateResource.IsValid())
		{
			Print(string.Format("TrainWreck: Invalid placeholder crate resource: %1", GetPlaceholderCratePrefab()), LogLevel.ERROR);
			return null;
		}
		
		if(crates.Contains(requestComponent.GetPlayerId()))
		{
			SCR_EntityHelper.DeleteEntityAndChildren(crates.Get(playerId).GetOwner());
			crates.Remove(playerId);
		}
		
		IEntity crateEntity = GetGame().SpawnEntityPrefab(crateResource, GetGame().GetWorld(), crateSpawnParams);
		
		if(!crateEntity)
		{
			Debug.Error("TrainWreck: Crate couldn't spawn");
			return null;
		}
		
		SCR_TW_PlayerCrateComponent crate = TW<SCR_TW_PlayerCrateComponent>.Find(crateEntity);
		
		if(!crate)
		{
			Debug.Error("TrainWreck: Could not locate SCR_TW_PlayerCrateComponent");
			return null;
		}
		
		crate.InitializeForPlayer(requestComponent.GetPlayerId());
		crates.Insert(requestComponent.GetPlayerId(), crate);
		
		return crate;
	}	
	
	//------------------------------------------------------------------------------------------------
	//! RPC Call to server to ensure only the server updates/saves inventory 
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcUpdatePlayerCrate(int playerId)
	{
		if(!crates.Contains(playerId))
			return;
		
		crates.Get(playerId).InitializeForPlayer(playerId);
	}
	
	void SaveAndDeleteCrate(int playerId)
	{
		if(!TW_Global.IsServer(GetOwner()))
			return;
		
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
	
	override void OnGameModeStart()
	{
		if(!TW_Global.IsInRuntime())
			return;
				
		if(!TW_Global.IsServer(GetOwner()))
			return;
		
		GetGame().GetCallqueue().CallLater(InitializePlayerHub, SCR_TW_Util.FromSecondsToMilliseconds(1), false);
		GetGame().GetCallqueue().CallLater(CheckPlayerWipe, SCR_TW_Util.FromSecondsToMilliseconds(15), true);
		
		TW_LootManager.InitializeLootTable();
		
		#ifdef WORKBENCH
		if(m_EnableLootSpawns)	
			TW_LootManager.SpawnLoot();
		#else
		TW_LootManager.SpawnLoot();
		#endif
	}
	
	void CallExtraction(TW_ExtractionType type)
	{
		if(!TW_Global.IsServer(GetOwner()))
		{
			Print("TrainWreck: CallExtraction has been invoked on the client. Error", LogLevel.ERROR);
			return;
		}
		
		ref array<int> playerIds = {};
		GetGame().GetPlayerManager().GetPlayers(playerIds);
		ref array<IEntity> players = {};
		foreach(int playerId : playerIds)
		{
			IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			if(player)
				players.Insert(player);
		}
		
		switch(type)
		{
			case TW_ExtractionType.STANDARD:
			{
				if(possibleExtractionSites.IsEmpty())
				{
					PrintFormat("TrainWreck: There are no standard spawn sites", LogLevel.ERROR);
					return;					
				}
				
				SCR_TW_ExtractionSite site = GetRandomExtractionSite();
				
				if(!site)
				{
					PrintFormat("TrainWreck: No standard extraction sites were found", LogLevel.ERROR);
					return;
				}	
				
				site.SpawnSite();
				PopUpMessage("Extraction", "A new extraction point is now available");
				break;
			}
		}
	}
	
	//! Get the furthest extraction site from list of players
	private SCR_TW_ExtractionSite GetFurthestSpawnPointFrom(notnull array<IEntity> players)
	{
		SCR_TW_ExtractionSite nearest;
		float distance = -1;
		
		foreach(IEntity player : players)
		foreach(SCR_TW_ExtractionSite site : possibleExtractionSites)
		{	
			if(site.IsOccupied())
				continue;
					
			if(!nearest)
			{
				nearest = site;
				continue;
			}						
			
			float span = vector.Distance(player.GetOrigin(), site.GetOrigin());
			
			if(span < distance)
				continue;
						
			distance = span;
			nearest = site;
		}
		
		return nearest;
	}
	
	private void CheckPlayerWipe()
	{
		// Players must have spawned in at least 1 time for things to be considered a wipe.
		// Otherwise a fresh server may end up restarting infinitely while no one is on 
		
		if(!playersHaveSpawned && !m_MatchOver)
			return;
		
		// Total connected players
		int playerCount = GetGame().GetPlayerManager().GetPlayerCount();
		
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
			Debug.Error("TrainWreck: No spawn points have been registered");
			return;
		}
		
		int insertionSpawnPointCount = Math.Min(numberOfInsertionPoints, possibleSpawnAreas.Count());
		
		for(int i = 0; i < insertionSpawnPointCount; i++)
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
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	void RpcDo_PlaySoundOnEntity(RplId entityID, string soundName)
	{
		if(!entityID)
			return;
		
		IEntity entity = TW_Global.GetEntityByRplId(entityID);
		
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
		
		RplComponent rpl = TW<RplComponent>.Find(entity);
		
		if(Replication.IsServer())
			Rpc(RpcDo_PlaySoundOnEntity, rpl.Id(), soundName);
		RpcDo_PlaySoundOnEntity(rpl.Id(), soundName);
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
		Print(string.Format("popup: %1 | %2", title, subtitle));
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
	
	void RpcAsk_DeleteItem(IEntity item)
	{
		RplComponent rplComponent = TW<RplComponent>.Find(item);
		
		if(!rplComponent)
		{
			Print("TrainWreck: RpcAsk_DeleteItem -> Item does not have replication component", LogLevel.ERROR);
			return;
		}
		
		RplId id = rplComponent.Id();
		
		SCR_EntityHelper.DeleteEntityAndChildren(item);
		Rpc(RpcDo_DeleteItem, id);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	private void RpcDo_DeleteItem(RplId itemId)
	{
		IEntity item = TW_Global.GetEntityByRplId(itemId);
		
		if(!item)
		{
			Print(string.Format("TrainWreck: RpcDo_DeleteItem --> Unable to locate item with replication ID %1", itemId), LogLevel.ERROR);
			return;
		}
		
		SCR_EntityHelper.DeleteEntityAndChildren(item);
	}
	
	void DeletePlayer(int playerId)
	{
		Rpc(RpcDo_DeletePlayerById, playerId);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RpcDo_DeletePlayerById(int playerId)
	{
		IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
		
		SCR_CharacterControllerComponent character = SCR_CharacterControllerComponent.Cast(player.FindComponent(SCR_CharacterControllerComponent));
		
		if(!character)
		{
			Debug.Error("TrainWreck: Unable to find character controller on player");
			return;
		}
		
		character.ForceDeath();
		
		if(player)
		{
			SCR_EntityHelper.DeleteEntityAndChildren(player);
			delete player;
			
			SCR_TW_ExtractionHandler.GetInstance().SaveAndDeleteCrate(playerId);
		}
	}
};