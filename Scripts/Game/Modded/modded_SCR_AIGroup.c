modded class SCR_AIGroup
{
	[Attribute("", UIWidgets.CheckBox, category: "Wandaring System", desc: "Should this AI group ignore wandering")]
	protected bool m_ExcludeFromWanderingSystem;
	
	void SetIgnoreWandering(bool value)
	{
		m_ExcludeFromWanderingSystem = value;
	}
	
	bool IgnoreWanderingSystem() { return m_ExcludeFromWanderingSystem; }
}