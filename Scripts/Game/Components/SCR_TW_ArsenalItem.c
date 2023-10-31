[BaseContainerProps(), SCR_BaseContainerCustomTitleResourceName("m_ItemResourceName", true)]
modded class SCR_ArsenalItem
{
	[Attribute("25", UIWidgets.Slider, "Chance to spawn", "0 100 1")]
	protected float m_ItemChanceToSpawn;
	
	[Attribute("1", UIWidgets.Slider, "Max # to Spawn", "1, 50 1")]
	protected int m_ItemMaxSpawnCount;
	
	protected ResourceName m_ItemPrefab;
	
	ResourceName GetItemPrefab() { return m_ItemPrefab; }
	void SetItemPrefab(ResourceName resource) { m_ItemPrefab = resource; }
	float GetItemChanceToSpawn() { return m_ItemChanceToSpawn; }
	int GetItemMaxSpawnCount() { return m_ItemMaxSpawnCount; }
};