[BaseContainerProps(configRoot:true)]
class TW_CompositionConfig : ScriptAndConfig
{
	[Attribute("", UIWidgets.EditBox, "Faction Key", category: "Basic Info")]
	private FactionKey m_pFactionKey;		
	
	[Attribute("", UIWidgets.ResourcePickerThumbnail, desc: "Small road site prefabs of controlled areas", category: "Sites: Road Prefabs")]
	private ref array<ResourceName> m_aSmallRoadSitePrefabs;
	
	[Attribute("", UIWidgets.ResourcePickerThumbnail, desc: "Medium road site prefabs of controlled areas", category: "Sites: Road Prefabs")]
	private ref array<ResourceName> m_aMediumRoadSitePrefabs;
	
	[Attribute("", UIWidgets.ResourcePickerThumbnail, desc: "Large road site prefabs of controlled areas", category: "Sites: Road Prefabs")]
	private ref array<ResourceName> m_aLargeRoadSitePrefabs;
	
	[Attribute("", UIWidgets.ResourcePickerThumbnail, desc: "Small site prefabs of controlled areas", category: "Sites: Site Prefabs")]
	private ref array<ResourceName> m_aSmallSitePrefabs;
	
	[Attribute("", UIWidgets.ResourcePickerThumbnail, desc: "Medium site prefabs of controlled areas", category: "Sites: Site Prefabs")]
	private ref array<ResourceName> m_aMediumSitePrefabs;
	
	[Attribute("", UIWidgets.ResourcePickerThumbnail, desc: "Large site prefabs of controlled areas", category: "Sites: Site Prefabs")]
	private ref array<ResourceName> m_aLargeSitePrefabs;
	
	[Attribute("1", UIWidgets.CheckBox, desc: "Cares about Points of Interest", category: "POI")]
	private bool m_CaresAboutPointsOfInterest;
	
	FactionKey GetFactionKey() { return m_pFactionKey; }
	bool CaresAboutPointsOfInterest() { return m_CaresAboutPointsOfInterest; }
	
	ResourceName GetRandomSmallCheckpointPrefab() 
	{ 
		if(!m_aSmallRoadSitePrefabs || m_aSmallRoadSitePrefabs.IsEmpty()) return ResourceName.Empty;
		return m_aSmallRoadSitePrefabs.GetRandomElement(); 
	}
	
	ResourceName GetRandomMediumCheckpointPrefab() 
	{ 
		if(!m_aMediumRoadSitePrefabs || m_aMediumRoadSitePrefabs.IsEmpty()) return ResourceName.Empty;
		return m_aMediumRoadSitePrefabs.GetRandomElement(); 
	}
	
	ResourceName GetRandomLargeCheckpointPrefab() 
	{ 
		if(!m_aLargeRoadSitePrefabs || m_aLargeRoadSitePrefabs.IsEmpty()) return ResourceName.Empty;
		return m_aLargeRoadSitePrefabs.GetRandomElement(); 
	}
	
	ResourceName GetRandomSmallSitePrefab() 
	{
		if(!m_aSmallSitePrefabs || m_aSmallSitePrefabs.IsEmpty()) return ResourceName.Empty;
		 return m_aSmallSitePrefabs.GetRandomElement(); 
	}
	
	ResourceName GetRandomMediumSitePrefab() 
	{ 
		if(!m_aMediumSitePrefabs || m_aMediumSitePrefabs.IsEmpty()) return ResourceName.Empty;
		return m_aMediumSitePrefabs.GetRandomElement(); 
	}
	
	ResourceName GetRandomLargeSitePrefab() 
	{ 
		if(!m_aLargeSitePrefabs || m_aLargeSitePrefabs.IsEmpty()) return ResourceName.Empty;
		return m_aLargeSitePrefabs.GetRandomElement(); 
	}
};