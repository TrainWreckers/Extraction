modded class SCR_AIGroup
{
	[Attribute("", UIWidgets.CheckBox, category: "Wandaring System", desc: "Should this AI group ignore wandering")]
	protected bool m_ExcludeFromWanderingSystem;
	
	[Attribute("", UIWidgets.CheckBox, category: "Global Count", desc: "Should this AI group be ignored in global count")]
	protected bool m_ExcludeFromGlobalCount;
	
	void SetIgnoreGlobalCount(bool value)
	{
		m_ExcludeFromGlobalCount = value;
	}
	
	void SetIgnoreWandering(bool value)
	{
		m_ExcludeFromWanderingSystem = value;
	}
	
	bool IgnoreGlobalCount() { return m_ExcludeFromGlobalCount; }
	bool IgnoreWanderingSystem() { return m_ExcludeFromWanderingSystem; }
}