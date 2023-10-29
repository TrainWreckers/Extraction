[EntityEditorProps(category: "GameScripted/TrainWreck/Systems", description: "Entity that takes care of managing composition spawning.", color: "0 0 255 255")]
class SCR_TW_AmbientEncountersClass: SCR_BaseGameModeComponentClass
{
	
};


class SCR_TW_AmbientEncounters: SCR_BaseGameModeComponent
{
	[Attribute("60", UIWidgets.Slider, params: "20, 300, 1", desc: "Number of seconds to check base ownership", category: "Bases")]
	private int baseSpawnCheckInterval;
	
	[Attribute("", UIWidgets.Object, params: "confg class=TW_FactionReinforcementsConfig", category: "Configs")]
	private ref TW_FactionReinforcementsConfig _opforConfig;
	
	[Attribute("", UIWidgets.Object, params: "confg class=TW_FactionReinforcementsConfig", category: "Configs")]
	private ref TW_FactionReinforcementsConfig _indforConfig;
	
	[Attribute("", UIWidgets.Object, params: "confg class=TW_FactionReinforcementsConfig", category: "Configs")]
	private ref TW_FactionReinforcementsConfig _bluforConfig;
	
	[Attribute("10", UIWidgets.Slider, params: "1 25 1", category: "Settings", desc: "Max Spawned Groups")]
	private int maxGroups;
	
	[Attribute("100", UIWidgets.Slider, params: "50, 3000, 50", category: "Settings", desc: "Minimum allowed spawn distance from nearest player")]
	private float minimumSpawnDistance;
	
	[Attribute("", UIWidgets.Object, params: "confg class=TW_GroupWaypoints", category: "Configs")]
	private ref TW_GroupWaypoints _waypointConfig;
	
	[Attribute("600", UIWidgets.Slider, params: "100 2000 20", desc: "Distance to start garbage collecting", category: "Garbage Collection")]
	private int garbageStartDistance;
	
	[Attribute("120", UIWidgets.Slider, params: "0 60 0.5", desc: "Time in minutes to start collecting a group outside of garbage distance", category: "Garbage Collection")]
	private float garbageStartMinutes;
	
	[Attribute(UIWidgets.Auto, category: "Encounters")]
	private ref array<ref TW_AmbientEncounterEntry> encounterConfigs;
	private ref map<SCR_TW_EncounterType, TW_AmbientEncounterEntry> encounterOptions = new ref map<SCR_TW_EncounterType, TW_AmbientEncounterEntry>;
	
	private ref array<SCR_AIGroup> spawnedTaskGroups = new ref array<SCR_AIGroup>;
	
	static ref RandomGenerator random = new RandomGenerator();
	
	private ref map<SCR_TW_EncounterType, bool> _cooldowns = new map<SCR_TW_EncounterType, bool>;
	private ref array<SCR_AIGroup> spawnedGroups = {};
	
	FactionKey opposingFaction = FactionKey.Empty;
	FactionKey playerFaction = FactionKey.Empty;
	
	private static SCR_TW_AmbientEncounters instance;
	static SCR_TW_AmbientEncounters GetInstance()
	{
		if(instance)
			return instance;
		
		instance = SCR_TW_AmbientEncounters.Cast(GetGame().GetGameMode().FindComponent(SCR_TW_AmbientEncounters));
		
		return instance;
	}
	
	private ref map<FactionKey, ref array<SCR_CampaignMilitaryBaseComponent>> factionBases = new ref map<FactionKey, ref array<SCR_CampaignMilitaryBaseComponent>>;
	private ref map<int, IEntity> players = new ref map<int, IEntity>;
	private ref array<IEntity> playerList = new ref array<IEntity>;
	private ref array<SCR_TW_EncounterHandler> _handlers = new ref array<SCR_TW_EncounterHandler>;
	
	protected override void OnPostInit(IEntity owner)
	{
		if(!GetGame().InPlayMode())
			return;
		
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if(rpl && !rpl.IsMaster())
			return;
		
		if(!_indforConfig)
		{
			Print("Encounter Handler requires having an Independent Faction Config", LogLevel.ERROR);
			return;
		}
		
		foreach(TW_AmbientEncounterEntry entry : encounterConfigs)
		{
			StartEncounterWithCooldown(entry.GetEncounterType(), entry.GetInitialCooldownPeriod());
			
			if(!encounterOptions.Contains(entry.GetEncounterType()))
				encounterOptions.Insert(entry.GetEncounterType(), entry);
			else
				encounterOptions.Set(entry.GetEncounterType(), entry);
		}
		
		Print(string.Format("Total Encounter Configs Loaded: %1", encounterOptions.Count()), LogLevel.NORMAL);
				
		GetGame().GetCallqueue().CallLater(CheckBases, SCR_TW_Util.FromSecondsToMilliseconds(baseSpawnCheckInterval), true);
		GetGame().GetCallqueue().CallLater(CheckHandlers, SCR_TW_Util.FromSecondsToMilliseconds(baseSpawnCheckInterval + 10), true);
		GetGame().GetCallqueue().CallLater(CheckPlayers, SCR_TW_Util.FromSecondsToMilliseconds(baseSpawnCheckInterval - 2), true);
	}
	
	// Ensures we have the latest player dictionary
	private void CheckPlayers()
	{
		ref array<int> playerIds = new ref array<int>;
		GetGame().GetPlayerManager().GetAllPlayers(playerIds);
		
		players.Clear();		
		playerList.Clear();
		
		foreach(int playerId : playerIds)
		{
			if(players.Contains(playerId)) continue;
			
			IEntity player = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
			
			if(!player)
				continue;
			
			players.Insert(playerId, player);
			playerList.Insert(player);
			
			if(!playerFaction || playerFaction == FactionKey.Empty)
			{
				FactionAffiliationComponent fac = FactionAffiliationComponent.Cast(player.FindComponent(FactionAffiliationComponent));
				
				if(!fac) continue;
				
				playerFaction = fac.GetAffiliatedFaction().GetFactionKey();
				
				if(playerFaction == _bluforConfig.GetFactionKey())
					opposingFaction = _opforConfig.GetFactionKey();
				else
					opposingFaction = _bluforConfig.GetFactionKey();		
			}
		}		
	}
	
	// This method updates the dictionary holding faction bases
	private void CheckBases()
	{
		Print("Ambient Encounters: Checking Base Ownership", LogLevel.DEBUG);
		
		ref map<FactionKey, ref array<SCR_CampaignMilitaryBaseComponent>> baseDict = new ref map<FactionKey, ref array<SCR_CampaignMilitaryBaseComponent>>;
		SCR_TW_Component.GetFactionBaseDict(baseDict);
		
		if(baseDict.IsEmpty())
		{
			Print("Was unable to retrieve bases...", LogLevel.ERROR);
			return;
		}
		
		factionBases = baseDict;
	}
	
	private void CheckHandlers()
	{
		Print("Checking Encounter Handlers...");				
	}
	
	private void CheckSpawnedGroups()
	{
		if(!spawnedGroups || spawnedGroups.IsEmpty()) return;
		
		Print(string.Format("Processing Ambient Encounter Groups: %1/%2", spawnedGroups.Count(), maxGroups), LogLevel.DEBUG);
		int deadGroups = 0;
		
		foreach(SCR_AIGroup group : spawnedGroups)
		{
			if(group == null)
			{
				spawnedGroups.RemoveItem(group);
				deadGroups++;
				continue;
			}
			
			if(group.GetAgentsCount() <= 0)
			{
				deadGroups++;
				spawnedGroups.RemoveItem(group);
			}
		}
		
		if(deadGroups > 0)
			Print(string.Format("Ambient Encounters removed %1 dead groups", deadGroups), LogLevel.DEBUG);
		
		Print(string.Format("Current Ambient Encounter Group Count %1 / %2", spawnedGroups.Count(), maxGroups), LogLevel.DEBUG);
	}
	
	void SpawnPatrol(SCR_CampaignMilitaryBaseComponent base)
	{
		Print(string.Format("Creating Ambient Patrol around: %1", base.GetBaseName()), LogLevel.DEBUG);
		
		FactionKey owningFaction = base.GetFaction().GetFactionKey();
		
		ResourceName patrolPrefab;
		
		if(owningFaction == _indforConfig.GetFactionKey() || owningFaction == FactionKey.Empty)
		{
			patrolPrefab = _indforConfig.GetRandomInfantryPrefab();	
		}
		else if(owningFaction == _bluforConfig.GetFactionKey())
		{
			patrolPrefab = _bluforConfig.GetRandomInfantryPrefab();
		}
		else
		{
			patrolPrefab = _opforConfig.GetRandomInfantryPrefab();
		}
		
		SCR_AIGroup group = SCR_TW_Util.SpawnGroup(patrolPrefab, base.GetOwner().GetOrigin(), 500);
		
		if(!group)
		{
			Print(string.Format("Ambient Encounters: <SpawnPatrol> Was unable to create patrol around %1", base.GetBaseName()), LogLevel.WARNING);
			return;
		}
		
		spawnedGroups.Insert(group);
		SCR_TW_Util.CreatePatrolPathFor(group, _waypointConfig.GetPatrolWaypointPrefab(), _waypointConfig.GetCycleWaypointPrefab(), Math.RandomInt(5, 20), 100);
	}
	
	void Process()
	{
		CheckSpawnedGroups();
		
		if(spawnedGroups.Count() >= maxGroups) return;
		
		float chance = random.RandFloat01();
		SCR_TW_EncounterType encounterType = GetRandomEncounter();
		
		Print(string.Format("Ambient Encounters: Type: %1", encounterType));
		IEntity randomPlayer = playerList.GetRandomElement();
		
		// Find the closest base that DOES NOT belong to the player faction
		SCR_CampaignMilitaryBaseComponent closestBase = SCR_TW_Component.GetCampaignBaseManager().FindClosestBaseNotFaction(playerFaction, randomPlayer.GetOrigin());		
		
		if(!encounterOptions.Contains(encounterType))
		{
			SpawnPatrol(closestBase);
		}
		else
		{
			SCR_TW_EncounterHandler handler = SpawnHandler(encounterOptions.Get(encounterType).GetEncounterPrefab());
			
			if(handler)
			{
				Print(string.Format("Encounter Handler created for: %1", encounterType), LogLevel.DEBUG);
				_handlers.Insert(handler);
			}
			else
				Print(string.Format("Was unable to create Encounter Handler for Encounter Type: %1", encounterType), LogLevel.ERROR);
		}
	}
	
	SCR_TW_EncounterHandler SpawnHandler(ResourceName prefab)
	{
		Resource encounterResource = Resource.Load(prefab);
		EntitySpawnParams params();
		vector mat[4];
		GetGameMode().GetTransform(mat);
		params.TransformMode = ETransformMode.WORLD;
		params.Transform = mat;
		
		SCR_TW_EncounterHandler handler = SCR_TW_EncounterHandler.Cast(GetGame().SpawnEntityPrefab(encounterResource, GetGame().GetWorld(), params));
		
		if(handler)
		{
			handler.Setup(factionBases, playerFaction, opposingFaction, playerList);
			if(handler.HasErrored())
			{
				Print(string.Format("Something went wrong while initializing %1 handler", prefab), LogLevel.ERROR);
				return null;
			}
		}
		else
		{
			Print(string.Format("Something went wrong. Was unable to create %1 handler", prefab), LogLevel.ERROR);
		}
		
		return handler;
	}
	
	bool IsValidSpawnPosition(vector startingLocation, notnull array<IEntity> positions, int minimumDistance)
	{
		foreach(IEntity entity : positions)
		{
			float distance = vector.Distance(startingLocation, entity.GetOrigin());
			if(distance < minimumDistance)
				return false;
		}
		
		return true;
	}
	
	private void StartEncounterWithCooldown(SCR_TW_EncounterType encounter, int minutes)
	{
		if(minutes <= 0)
			return;
		
		_cooldowns.Insert(encounter, false);
		GetGame().GetCallqueue().CallLater(ResetCooldownForEncounter, minutes, false, encounter);
	}
	
	private void ResetCooldownForEncounter(SCR_TW_EncounterType encounterType)
	{
		Print(string.Format("Encounter Type: %1 -- cooldown period over", encounterType.ToString()), LogLevel.DEBUG);
		_cooldowns.Set(encounterType, true);
	}
	
	void DisplayText(string title, string description, int seconds, string sound)
	{
		SCR_PopUpNotification.GetInstance().PopupMsg(title, SCR_TW_Util.FromSecondsToMilliseconds(seconds), text2: description, sound: sound);
		SCR_UISoundEntity.SoundEvent(sound);
		Rpc(GlobalHint, title, description, seconds, sound);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Broadcast)]
	private void GlobalHint(string title, string description, int seconds, string sound)
	{
		SCR_PopUpNotification.GetInstance().PopupMsg(title, SCR_TW_Util.FromSecondsToMilliseconds(seconds), text2: description, sound: sound);
		SCR_UISoundEntity.SoundEvent(sound);
	}
	
	private SCR_TW_EncounterType GetRandomEncounter()
	{
		ref array<SCR_TW_EncounterType> types = new ref array<SCR_TW_EncounterType>;
		
		foreach(TW_AmbientEncounterEntry entry : encounterConfigs)
		{
			if(_cooldowns.Contains(entry.GetEncounterType()))
			{
				if(!_cooldowns.Get(entry.GetEncounterType()))
					continue;
			}
			
			if(entry.GetChance())
				types.Insert(entry.GetEncounterType());			
		}
		
		if(types.IsEmpty())
			return null;
		
		return types.GetRandomElement();
	}
	
	TW_FactionReinforcementsConfig GetBluforConfig()
	{
		return _bluforConfig;
	}
	
	TW_FactionReinforcementsConfig GetOpforConfig()
	{
		return _opforConfig;
	}
	
	TW_FactionReinforcementsConfig GetIndforConfig()
	{
		return _indforConfig;
	}
	
	
	// Distance away from nearest player = needs to get yeeted
	private ref map<int, SCR_AIGroup> gcGroups = new ref map<int, SCR_AIGroup>;
	
	private void HandleGroupGC(SCR_AIGroup group)
	{
		if(group == null)
			return;
		
		// Does it still fit the criteria for being garbage collected after wait time?	
		if(!CanBeGC(group))
		{
			gcGroups.Remove(group.GetGroupID());
			return;
		}
		
		// Delete Group
		int groupId = group.GetGroupID();
		SCR_TW_Util.DeleteGroup(group);
		gcGroups.Remove(groupId);
	}
	
	private bool CanBeGC(SCR_AIGroup group)
	{
		foreach(IEntity player : playerList)
		{
			float distance = vector.Distance(player.GetOrigin(), group.GetOrigin());
			
			if(distance <= garbageStartDistance)
				return false;
		}
		
		return true;
	}
		
	void RegisterForGarbageCollection(notnull array<SCR_AIGroup> groups)
	{
		foreach(SCR_AIGroup group : groups)
		{
			if(!group)
				continue;
			
			// If we're already tracking this group, no need to check again
			if(gcGroups.Contains(group.GetGroupID()))
				continue;
			
			if(!gcGroups.Contains(group.GetGroupID()) && CanBeGC(group))
			{
				gcGroups.Insert(group.GetGroupID(), group);
				GetGame().GetCallqueue().CallLater(HandleGroupGC, SCR_TW_Util.FromMinutesToMilliseconds(garbageStartMinutes), false, group);
			}
		}
	}
};