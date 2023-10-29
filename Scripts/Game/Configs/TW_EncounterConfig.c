[BaseContainerProps(configRoot: true)]
class TW_EncounterConfig : ScriptAndConfig
{	
	[Attribute("1", UIWidgets.Slider, params: "1 50 1", category: "Spawn Count")]
	protected int m_Min;
	
	[Attribute("5", UIWidgets.Slider, params: "1 50 1", category: "Spawn Count")]
	protected int m_Max;
	
	[Attribute("", UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(SCR_TW_EncounterType))]
	protected SCR_TW_EncounterType m_EncounterType;
	
	[Attribute("300", UIWidgets.Slider, params: "0 1000 5", category: "Distance")]
	protected int m_MinimumSpawnDistance;
	
	[Attribute("", UIWidgets.ResourceNamePicker)]
	protected ResourceName m_TaskPrefab;
	
	[Attribute("3", UIWidgets.Slider, params: "0 30 1", category: "Cooldown")]
	protected int m_MinimumCooldownPeriod;
	
	[Attribute("10", UIWidgets.Slider, params: "0 30 1", category: "Cooldown")]
	protected int m_MaximumCooldownPeriod;	
	
	// Retrieve the cooldown period for this encounter.
	int GetCooldownPeriod() { return Math.RandomIntInclusive(m_MinimumSpawnDistance, m_MaximumCooldownPeriod); }
	
	// Retrieve the type of encounter this represents
	SCR_TW_EncounterType GetEncounterType() { return m_EncounterType; }
	
	// Retrieve the task prefab to use for assigning players/showing on the map
	ResourceName GetTaskPrefab() { return m_TaskPrefab; }
	
	// Generate a random number of groups to spawn
	int GenerateSpawnCount() { return Math.RandomIntInclusive(m_Min, m_Max); }
	
	// Get the minimum spawn distance that is allowed for spawning groups
	int GetMinimumSpawnDistance() { return m_MinimumSpawnDistance; }	
};


enum TW_ESeizeEncounterPhase {
	Reinforce,
	LaunchAttack,
	Preparation
}

[BaseContainerProps(configRoot: true)]
class TW_SeizeEncounterConfig : TW_EncounterConfig
{
	[Attribute("2", UIWidgets.Slider, params: "1 30 1", category: "Reinforce Phase Timer")]
	protected int m_MinReinforceTimer;
	
	[Attribute("5", UIWidgets.Slider, params: "1 30 1", category: "Reinforce Phase Timer")]
	protected int m_MaxReinforceTimer;
	
	[Attribute("1", UIWidgets.Slider, params: "1 5 1", category: "Spawn Directions")]
	protected int m_MinDirections;
	
	[Attribute("3", UIWidgets.Slider, params: "1 5 1", category: "Spawn Directions")]
	protected int m_MaxDirections;
	
	int GenerateReinforceTimer() { return Math.RandomIntInclusive(m_MinReinforceTimer, m_MaxReinforceTimer); }
	int GenerateDirectionCount() { return Math.RandomIntInclusive(m_MinDirections, m_MaxDirections); }	
};