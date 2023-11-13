[EntityEditorProps(category: "GameScripted/TrainWreck/Systems", description: "Extraction Point for players")]
class SCR_TW_ExtractionSiteClass: SCR_SiteSlotEntityClass
{
	
};

class SCR_TW_ExtractionSite : SCR_SiteSlotEntity
{
	[Attribute("", UIWidgets.ResourcePickerThumbnail, category: "Prefabs", params: "et", desc: "Composition prefabs")]
	private ref array<ResourceName> extractionSitePrefabs;
	
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
		
		SpawnEntityInSlot(resource);
	}
	
	override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);
		
		if(!GetGame().InPlayMode())
			return;
		
		SCR_TW_ExtractionHandler.GetInstance().RegisterExtractionSite(this);				
	}
};