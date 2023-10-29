[BaseContainerProps(configRoot: true)]
class TW_CounterAttackConfig : ScriptAndConfig
{
	[Attribute("FIA", UIWidgets.EditBox, category: "Basic Info")]
	protected FactionKey m_FactionKey;
	
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "Group Prefabs to spawn", params: "et", category: "Basic Info")]
	private ref array<ResourceName> m_GroupPrefabs;
	
	[Attribute("", UIWidgets.Slider, params: "0 20 1", desc: "Minimum number of groups to spawn for counter attack", category: "Groups")]
	private int m_MinGroups;
	
	[Attribute("", UIWidgets.Slider, params: "1 20 1", desc: "Maximum number of groups to spawn for counter attack", category: "Groups")]
	private int m_MaxGroups;
	
	[Attribute("1", UIWidgets.Slider, params: "0 1 .05", desc: "Number between 0 and 1 indicating chance of a counter attack occurring", category: "Basic Info")]
	private float m_ChanceOfOccurring;
	
	[Attribute("1", UIWidgets.Slider, params: "0 5 1", desc: "Minimum number of directions groups will come from", category: "Directions")]
	private int m_MinDirections;
	
	[Attribute("1", UIWidgets.Slider, params: "1 5 1", desc: "Maximum number of directions groups will come from", category: "Directions")]
	private int m_MaxDirections;
	
	FactionKey GetFactionKey() { return m_FactionKey; }
	
	ResourceName GetRandomGroupPrefab()
	{
		if(!m_GroupPrefabs || m_GroupPrefabs.IsEmpty()) return ResourceName.Empty;
		
		return m_GroupPrefabs.GetRandomElement();
	}
	
	float GetChanceOfOccurring() {return m_ChanceOfOccurring;}
	int GetMinGroups() { return m_MinGroups; }
	int GetMaxGroups() { return m_MaxGroups; }
};