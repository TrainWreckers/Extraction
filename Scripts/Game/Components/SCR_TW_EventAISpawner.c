[EntityEditorProps(category: "GameScripted/TrainWreck/Systems", description: "Event sites for Extraction GameMode")]
class SCR_TW_EventAISpawnerClass : GenericEntityClass
{
	
};

class SCR_TW_EventAISpawner : GenericEntity
{
	[Attribute("", UIWidgets.Auto, category: "Scavengers", params: "et")]
	private ref array<ResourceName> m_ScavengerPrefabs;
	
	[Attribute("", UIWidgets.Auto, category: "Military", params: "et")]
	private ref array<ResourceName> m_MilitaryPrefabs;
	
	[Attribute("", UIWidgets.Auto, category: "Waypoints", params: "et")]
	private ref array<ResourceName> m_WaypointPrefabs;
	
	//! Spawn a singular prefab based on boolean value. 
	void SpawnUnit(bool spawnScavs = false)
	{
		ResourceName prefab;
		
		// Prevent breaking if scavenger prefab(s) aren't provided
		if(spawnScavs && !m_ScavengerPrefabs.IsEmpty())
			prefab = m_ScavengerPrefabs.GetRandomElement();
		else
			prefab = m_MilitaryPrefabs.GetRandomElement();
		
		SCR_AIGroup group = SCR_TW_Util.SpawnGroup(prefab, GetOrigin(), 1);
		
		if(!group)
		{
			Print(string.Format("TrainWreck: Was unable to spawn %1", prefab), LogLevel.ERROR);
			return;
		}
		
		group.SetIgnoreWandering(!spawnScavs);
		group.SetIgnoreGlobalCount(!spawnScavs);
		
		if(m_WaypointPrefabs.IsEmpty())
			return;
		
		ResourceName waypointPrefab = m_WaypointPrefabs.GetRandomElement();
		AIWaypoint waypoint = SCR_TW_Util.CreateWaypointAt(waypointPrefab, GetOrigin());
		
		if(waypoint)
			group.AddWaypoint(waypoint);
	}
};