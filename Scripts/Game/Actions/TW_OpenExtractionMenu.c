class TW_OpenExtractionMenuAction : ScriptedUserAction
{
	[Attribute("", UIWidgets.ResourcePickerThumbnail, category: "Extraction", desc: "Required item user must have in order to call in standard extraction")]	
	protected ResourceName m_RequiredToUserStandard;
	
	[Attribute("", UIWidgets.ResourcePickerThumbnail, category: "Extraction", desc: "Users with this item gain access to higher tier extractions")]	
	protected ResourceName m_HighTierRadio;		
	
	override void PerformAction(IEntity pOwnerEntity, IEntity pUserEntity)
	{
		MenuBase menuBase = GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.TW_CallExtractionMenu);
		
		TW_UI_ExtractionDisplay menu = TW_UI_ExtractionDisplay.Cast(menuBase);	
	}
	
	override bool CanBePerformedScript(IEntity user)
	{
		return true;
	}
	
	override bool CanBeShownScript(IEntity user)
	{
		return true;
	}
};