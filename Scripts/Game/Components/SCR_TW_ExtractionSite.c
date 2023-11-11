class SCR_TW_ExtractionSiteComponentClass: ScriptComponentClass
{
	
};

class SCR_TW_ExtractionSiteComponent : ScriptComponent
{
	override void OnPostInit(IEntity owner)
	{		
		super.EOnInit(owner);
		
		if(!GetGame().InPlayMode())
			return;
		
		auto slot = SCR_SiteSlotEntity.Cast(owner);
		
		if(!slot)
		{
			Print("TrainWreck: SCR_TW_ExtractionSiteComponent requires a SCR_SiteSlotEntity", LogLevel.ERROR);
			return;
		}
		
		SCR_TW_ExtractionHandler.GetInstance().RegisterExtractionSite(this);
	}
	

};