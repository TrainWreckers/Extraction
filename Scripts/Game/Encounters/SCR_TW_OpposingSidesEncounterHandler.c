[EntityEditorProps(category: "GameScripted/TrainWreck/Encounters", description: "Handles opposing side based encounters")]
class SCR_TW_OpposingSidesEncounterHandlerClass : SCR_TW_EncounterHandlerClass 
{

};

class SCR_TW_OpposingSidesEncounterHandler : SCR_TW_EncounterHandler
{		
	override void Setup(notnull map<FactionKey, ref array<SCR_CampaignMilitaryBaseComponent>> baseDict, FactionKey playerFaction, FactionKey opposingFaction, notnull array<IEntity> players)
	{
		if(!IsMaster())
		{
			m_HasErrored = true;
			return;
		}
		
		super.Setup(baseDict, playerFaction, opposingFaction, players);				
		
		float chance = Math.RandomFloat(0, 100);
		
		if(chance <= 50)
			SpawnGroupsAroundPlayers();
		else
			SpawnGroupsAroundIndepBases();
	}
	
	override void SelectGroupConfig()
	{
		if(playerFaction == SCR_TW_AmbientEncounters.GetInstance().GetOpforConfig().GetFactionKey())
			m_GroupPrefabsConfig = SCR_TW_AmbientEncounters.GetInstance().GetOpforConfig();
		else
			m_GroupPrefabsConfig = SCR_TW_AmbientEncounters.GetInstance().GetBluforConfig();
	}
	
	private void SpawnGroupsAroundPlayers()
	{
		int spawnCount = GetEncounterConfig().GenerateSpawnCount();
		
		IEntity player = players.GetRandomElement();
		
		if(!player)
			return;
		
		for(int i = 0; i < spawnCount; i++)
		{
			vector position = SCR_TW_Util.RandomPositionAround(player, GetEncounterConfig().GetMinimumSpawnDistance() * 1.5, GetEncounterConfig().GetMinimumSpawnDistance());
			SCR_AIGroup group = SCR_TW_Util.SpawnGroup(m_GroupPrefabsConfig.GetRandomInfantryPrefab(), position, 10);
			
			if(!group)
			{
				spawnCount--;
				Print("Was unable to spawn group for opposing sides around the player", LogLevel.WARNING);
				continue;
			}
			
			vector waypointPosition = SCR_TW_Util.RandomPositionAround(player, 100);
			AIWaypoint waypoint = SCR_TW_Util.CreateWaypointAt(GetWaypointsConfig().GetAttackWaypointPrefab(), waypointPosition);
			
			if(!waypoint)
			{
				SCR_TW_Util.DeleteGroup(group);
				continue;
			}
			
			group.AddWaypoint(waypoint);
			m_SpawnedGroups.Insert(group);
		}
		
		Print(string.Format("%1 Opposing Faction squads were spawned and heading twoards player location(s)", m_SpawnedGroups.Count()), LogLevel.DEBUG);
	}
	
	private void SpawnGroupsAroundIndepBases()
	{
		int spawnCount = GetEncounterConfig().GenerateSpawnCount();
		
		SCR_CampaignMilitaryBaseComponent base = SCR_TW_Util.GetSpawnLocation(baseDict.Get(opposingFaction), players, 2, GetEncounterConfig().GetMinimumSpawnDistance() * 1.5);
		
		for(int i = 0; i < spawnCount; i++)
		{
			vector position = SCR_TW_Util.RandomPositionAroundButAlso(base.GetOwner(), players, GetEncounterConfig().GetMinimumSpawnDistance() * 1.5, GetEncounterConfig().GetMinimumSpawnDistance());
			
			SCR_AIGroup group = SCR_TW_Util.SpawnGroup(m_GroupPrefabsConfig.GetRandomInfantryPrefab(), position, 10);
			if(!group)
			{
				Print("Was unable to spawn group for opposing sides encounter", LogLevel.WARNING);
				spawnCount--;
				continue;
			}
			
			vector waypointPosition = SCR_TW_Util.RandomPositionAround(base.GetOwner(), 300);
			AIWaypoint waypoint = SCR_TW_Util.CreateWaypointAt(GetWaypointsConfig().GetAttackWaypointPrefab(), waypointPosition);
			
			if(!waypoint)
			{
				SCR_TW_Util.DeleteGroup(group);
				continue;
			}
			
			group.AddWaypoint(waypoint);
			m_SpawnedGroups.Insert(group);
		}
		
		Print(string.Format("%1 Opposing Faction Squads have been spawned and heading towards %2", m_SpawnedGroups.Count(), base.GetBaseName()), LogLevel.DEBUG);
	}
};