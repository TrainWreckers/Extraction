[BaseContainerProps(configRoot: true)]
class TW_GroupWaypoints
{
	[Attribute("{B3E7B8DC2BAB8ACC}Prefabs/AI/Waypoints/AIWaypoint_SearchAndDestroy.et", UIWidgets.ResourceNamePicker, params: "et", category: "Waypoints", desc: "Groups assigned to attack are given this waypoint type")]
	private ResourceName m_AttackWaypointPrefab;
	
	[Attribute("{22A875E30470BD4F}Prefabs/AI/Waypoints/AIWaypoint_Patrol.et", UIWidgets.ResourceNamePicker, params: "et", category: "Waypoints", desc: "Groups assigned to patrol are given this waypoint type")]
	private ResourceName m_PatrolWaypointPrefab;
	
	[Attribute("{93291E72AC23930F}Prefabs/AI/Waypoints/AIWaypoint_Defend.et", UIWidgets.ResourceNamePicker, params: "et", category: "Waypoints", desc: "Groups assigned to defend are given this waypoint type")]
	private ResourceName m_DefendWaypointPrefab;
	
	[Attribute("{35BD6541CBB8AC08}Prefabs/AI/Waypoints/AIWaypoint_Cycle.et", UIWidgets.ResourceNamePicker, params: "et", category: "Waypoints", desc: "AI are given this waypoint when waypoints should repeat")]
	private ResourceName m_CycleWaypointPrefab;
	
	ResourceName GetAttackWaypointPrefab() { return m_AttackWaypointPrefab; }
	ResourceName GetPatrolWaypointPrefab() { return m_PatrolWaypointPrefab; }
	ResourceName GetDefendWaypointPrefab() { return m_DefendWaypointPrefab; }
	ResourceName GetCycleWaypointPrefab() { return m_CycleWaypointPrefab; }	
}