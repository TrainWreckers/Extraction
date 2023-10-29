[EntityEditorProps(category: "GameScripted/TrainWreck/Encounters", description: "Handles opposing side based encounters")]
class SCR_TW_AlliesEncounterHandlerClass : SCR_TW_EncounterHandlerClass 
{

};

class SCR_TW_AlliesEncounterHandler : SCR_TW_EncounterHandler
{
	override void SelectGroupConfig()
	{
		if(playerFaction == SCR_TW_AmbientEncounters.GetInstance().GetBluforConfig().GetFactionKey())
			m_GroupPrefabsConfig = SCR_TW_AmbientEncounters.GetInstance().GetBluforConfig();
		else
			m_GroupPrefabsConfig = SCR_TW_AmbientEncounters.GetInstance().GetOpforConfig();
	}
		
	override void Setup(notnull map<FactionKey, ref array<SCR_CampaignMilitaryBaseComponent>> baseDict, FactionKey playerFaction, FactionKey opposingFaction, notnull array<IEntity> players)
	{
		if(!IsMaster())
		{
			m_HasErrored = true;
			return;
		}
		
		super.Setup(baseDict, playerFaction, opposingFaction, players);						
				
		float chance = Math.RandomFloat(0, 100);
		
		if(chance < 50)
			SpawnGroupsForBaseDefense();			
		else
			SpawnGroupsForAttack();
		
		int cooldown = GetEncounterConfig().GetCooldownPeriod() * 3;
		GetGame().GetCallqueue().CallLater(UpdateOrders, delay: SCR_TW_Util.FromMinutesToMilliseconds(cooldown), repeat: false);
	}
	
	private void UpdateOrders()
	{
		Print("Updating waypoints for allies, to be around player location", LogLevel.DEBUG);
		
		IEntity player = players.GetRandomElement();
		
		if(!player)
		{
			Print("Ally encounter could not locate a valid player. Exiting...", LogLevel.WARNING);
			return;
		}
		
		foreach(SCR_AIGroup group : m_SpawnedGroups)
		{
			if(!group)
			{
				m_SpawnedGroups.RemoveItem(group);
				continue;
			}
			
			RemoveWaypoints(group);
			AIWaypoint waypoint = SCR_TW_Util.CreateWaypointAt(GetWaypointsConfig().GetAttackWaypointPrefab(), SCR_TW_Util.RandomPositionAroundPoint(player.GetOrigin(), 100));
			if(!waypoint)
			{
				SCR_TW_Util.DeleteGroup(group);
				m_SpawnedGroups.RemoveItem(group);
				continue;
			}
			
			group.AddWaypoint(waypoint);
		}
		
		int cooldown = GetEncounterConfig().GetCooldownPeriod();
		GetGame().GetCallqueue().CallLater(UpdateOrders, delay: SCR_TW_Util.FromMinutesToMilliseconds(cooldown), repeat: false);
	}
	
	private void SpawnGroupsForBaseDefense()
	{
		int spawnCount = GetEncounterConfig().GenerateSpawnCount();
		
		SCR_CampaignMilitaryBaseComponent playerBase = baseDict.Get(playerFaction).GetRandomElement();
		
		if(!playerBase)
		{
			Print("Was unable to find a suitable player base to defend. Switching to attack", LogLevel.WARNING);
			SpawnGroupsForAttack();
			return;
		}
		
		SCR_TW_AmbientEncounters.GetInstance().DisplayText("Reinforcements", string.Format("Reinforcements are enroute to %1", playerBase.GetBaseName()), 15, SCR_SoundEvent.SOUND_CP_POSITIVEFEEDBACK);
		
		for(int i = 0; i < spawnCount; i++)
		{
			vector position = SCR_TW_Util.RandomPositionAround(playerBase.GetOwner(), GetEncounterConfig().GetMinimumSpawnDistance() * 1.5, GetEncounterConfig().GetMinimumSpawnDistance());
			SCR_AIGroup group = SCR_TW_Util.SpawnGroup(m_GroupPrefabsConfig.GetRandomInfantryPrefab(), position, 10);
			
			if(!group)
			{
				spawnCount--;
				Print("Was unable to create ally group for ally encounter", LogLevel.WARNING);
				continue;
			}
			
			float chance = Math.RandomFloat(0, 100);
			ResourceName prefab;
			
			if(chance <= 50)
				prefab = GetWaypointsConfig().GetAttackWaypointPrefab();
			else
				prefab = GetWaypointsConfig().GetDefendWaypointPrefab();
			
			AIWaypoint waypoint = SCR_TW_Util.CreateWaypointAt(prefab, SCR_TW_Util.RandomPositionAround(playerBase.GetOwner(), 100));
			
			if(!waypoint)
			{
				Print("Was unable to create waypoint for ally group", LogLevel.WARNING);
				SCR_TW_Util.DeleteGroup(group);
				continue;
			}
			
			group.AddWaypoint(waypoint);
			m_SpawnedGroups.Insert(group);
		}
	}
	
	private void SpawnGroupsForAttack()
	{
		int spawnCount = GetEncounterConfig().GenerateSpawnCount();
		SCR_CampaignMilitaryBaseComponent enemyBase = SCR_TW_Util.GetSpawnLocation(baseDict.Get(opposingFaction), players, 2, GetEncounterConfig().GetMinimumSpawnDistance());
		SCR_CampaignMilitaryBaseComponent closestPlayerBase = SCR_TW_Util.GetClosestLocationToPlayer(baseDict.Get(playerFaction), players, GetEncounterConfig().GetMinimumSpawnDistance());
		
		for(int i = 0; i < spawnCount; i++)
		{
			vector position;
			
			if(!closestPlayerBase)
				position = SCR_TW_Util.RandomPositionAround(enemyBase.GetOwner(), GetEncounterConfig().GetMinimumSpawnDistance() * 2.5, GetEncounterConfig().GetMinimumSpawnDistance() * 2);
			else
				position = SCR_TW_Util.RandomPositionAround(closestPlayerBase.GetOwner(), GetEncounterConfig().GetMinimumSpawnDistance());
			
			SCR_AIGroup group = SCR_TW_Util.SpawnGroup(m_GroupPrefabsConfig.GetRandomInfantryPrefab(), position, 10);
			if(!group)
			{
				spawnCount--;
				Print("Something went wrong while trying to spawn ally group for attack", LogLevel.WARNING);
				continue;
			}
			
			AIWaypoint waypoint = SCR_TW_Util.CreateWaypointAt(GetWaypointsConfig().GetAttackWaypointPrefab(), SCR_TW_Util.RandomPositionAround(enemyBase.GetOwner(), 100));
			
			if(!waypoint)
			{
				Print("Something went wrong creating a waypoint for the attacking ally group", LogLevel.WARNING);
				SCR_TW_Util.DeleteGroup(group);
				continue;
			}
			
			group.AddWaypoint(waypoint);
			m_SpawnedGroups.Insert(group);
			
		}
	}
	
	private void RemoveWaypoints(SCR_AIGroup group)
	{
		ref array<AIWaypoint> waypoints = new ref array<AIWaypoint>;
		group.GetWaypoints(waypoints);
		
		foreach(AIWaypoint waypoint : waypoints)
		{
			group.RemoveWaypointFromGroup(waypoint);
		}
	}
};