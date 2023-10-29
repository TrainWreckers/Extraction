[BaseContainerProps(configRoot: true)]
class TW_FactionReinforcementsConfig : ScriptAndConfig
{
	[Attribute("", UIWidgets.EditBox, "Faction Key", category: "Basic Info")]
	private FactionKey m_FactionKey;
	
	[Attribute("", UIWidgets.ResourcePickerThumbnail, desc: "Light Infantry Groups", params: "et", category: "Infantry")]
	private ref array<ResourceName> m_LightInfantryGroups;
	
	[Attribute("", UIWidgets.ResourcePickerThumbnail, desc: "Weapons Infantry Groups", params: "et", category: "Infantry")]
	private ref array<ResourceName> m_WeaponsInfantryGroups;
	
	[Attribute("", UIWidgets.ResourcePickerThumbnail, desc: "Infantry Groups", params: "et", category: "Infantry")]
	private ref array<ResourceName> m_InfantryGroups;
	
	FactionKey GetFactionKey() { return m_FactionKey; }
	
	bool HasInfantryPrefabs()
	{
		return m_InfantryGroups && !m_InfantryGroups.IsEmpty();
	}
	
	bool HasLightInfantryPrefabs()
	{
		return m_LightInfantryGroups && !m_LightInfantryGroups.IsEmpty();		
	}
	
	bool HasWeaponsInfantryPrefabs()
	{
		return m_WeaponsInfantryGroups && !m_WeaponsInfantryGroups.IsEmpty();
	}
	
	ResourceName GetRandomLightInfantryPrefab()
	{
		if(HasLightInfantryPrefabs())
			return m_LightInfantryGroups.GetRandomElement();
		
		return ResourceName.Empty;
	}
	
	ResourceName GetRandomWeaponsInfantryPrefab()
	{
		if(HasWeaponsInfantryPrefabs())
			return m_WeaponsInfantryGroups.GetRandomElement();
		return ResourceName.Empty;
	}
	
	ResourceName GetRandomInfantryPrefab()
	{
		if(HasInfantryPrefabs())
			return m_InfantryGroups.GetRandomElement();
		
		return ResourceName.Empty;
	}
}