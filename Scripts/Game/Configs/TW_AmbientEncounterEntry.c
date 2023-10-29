[BaseContainerProps(configRoot: true)]
class TW_AmbientEncounterEntry : ScriptAndConfig
{
	[Attribute("0.5", UIWidgets.Slider, params: "0 1 0.05", desc: "Chance this encounter may happen", category: "Settings")]
	private float chance;
	
	[Attribute("", UIWidgets.ComboBox, enums: ParamEnumArray.FromEnum(SCR_TW_EncounterType), category: "Settings")]
	private SCR_TW_EncounterType m_EncounterType;
	
	[Attribute("10", UIWidgets.Slider, params: "0 30 1", category: "Cooldown", desc: "Initial cooldown period before this encounter can be used. In Minutes")]
	private int initialCooldown;
	
	[Attribute("", UIWidgets.ResourceNamePicker, params: "et", category: "Prefab", desc: "Prefab for encounter handler to spawn")]
	private ResourceName encounterPrefab;
	
	// Initial cooldown period for which this encounter cannot be used
	float GetInitialCooldownPeriod() { return initialCooldown; }
	
	// Get the actual chance value specified via config/editor
	bool GetChance() { return Math.RandomFloat01() <= chance; }
	
	// Which encounter is this associated with
	SCR_TW_EncounterType GetEncounterType() { return m_EncounterType; }
	
	ResourceName GetEncounterPrefab() { return encounterPrefab; }
};