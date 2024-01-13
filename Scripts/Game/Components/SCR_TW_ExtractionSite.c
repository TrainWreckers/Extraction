[EntityEditorProps(category: "GameScripted/TrainWreck/Systems", description: "Extraction Point for players")]
class SCR_TW_ExtractionSiteClass: SCR_SiteSlotEntityClass
{
	
};

class SCR_TW_ExtractionSite : SCR_SiteSlotEntity
{
	[Attribute("", UIWidgets.ResourcePickerThumbnail, category: "Prefabs", params: "et", desc: "Composition prefabs")]
	private ref array<ResourceName> extractionSitePrefabs;
	
	[Attribute("", UIWidgets.ResourceNamePicker, category: "Tasks", params: "et", desc: "Trigger to spawn")]
	private ResourceName m_TriggerPrefab;
	
	private SCR_TW_TriggerArea m_Trigger;
	
	//! Return trigger area from occupied extraction area/site (if available)
	SCR_TW_TriggerArea GetTriggerArea() { return m_Trigger; }
	
	bool UsesTrigger() { return m_TriggerPrefab != ResourceName.Empty; }
	
	void SpawnSite()
	{
		if(IsOccupied())
			return;
		
		if(extractionSitePrefabs.IsEmpty())
			return;
		
		ResourceName randomPrefab = extractionSitePrefabs.GetRandomElement();
		
		if(randomPrefab == ResourceName.Empty)
			return;
		
		Resource resource = Resource.Load(randomPrefab);
		if(!resource.IsValid())
			return;
		
		IEntity prefab = SpawnEntityInSlot(resource);
		
		// Should a trigger spawn here?
		if(m_TriggerPrefab == ResourceName.Empty)
			return;
		
		Resource triggerResource = Resource.Load(m_TriggerPrefab);
		EntitySpawnParams params = EntitySpawnParams();
		GetTransform(params.Transform);
		m_Trigger = SCR_TW_TriggerArea.Cast(GetGame().SpawnEntityPrefab(triggerResource, GetGame().GetWorld(), params));
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		if(!TW_Global.IsInRuntime())
			return;
		
		SCR_TW_ExtractionHandler.GetInstance().RegisterExtractionSite(this);				
	}
};