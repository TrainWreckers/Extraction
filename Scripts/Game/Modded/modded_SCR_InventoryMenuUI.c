modded class SCR_RadioComponent
{
	ERadioType GetRadioType()
	{
		return m_iRadioType;
	}	
};

modded class SCR_InventoryMenuUI
{
	private ERadioType m_SelectedRadio;
	
	override void NavigationBarUpdate()
	{
		super.NavigationBarUpdate();
		
		if(!m_pFocusedSlotUI)
			return;
		
		InventoryItemComponent itemComp = m_pFocusedSlotUI.GetInventoryItemComponent();
		
		if(!itemComp) return;
		
		SCR_RadioComponent radioComp = SCR_RadioComponent.Cast(itemComp.GetOwner().FindComponent(SCR_RadioComponent));
		
		if(!radioComp)
			return;
		
		m_SelectedRadio = radioComp.GetRadioType();
			
		// Should only appear for radios
		if(!itemComp || !itemComp.GetOwner().FindComponent(BaseRadioComponent))
			return;
		
		m_pNavigationBar.SetButtonEnabled("ButtonCallExtract", true);
	}
	
	override void OnAction( SCR_InputButtonComponent comp, string action, SCR_InventoryStorageBaseUI pParentStorage = null, int traverseStorageIndex = -1 )
	{
		PrintFormat("TrainWreck: Action -> %1", action);
		
		switch(action)
		{
			case "TW_OpenExtractionMenu":
			{
				Print("TrainWreck: Extracting");
				MenuBase menuBase = GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.TW_CallExtractionMenu);
				TW_UI_ExtractionDisplay menu = TW_UI_ExtractionDisplay.Cast(menuBase);
				menu.SetRadioType(m_SelectedRadio);	
				PrintFormat("TrainWreck: Radio Type: %1", m_SelectedRadio);
				break;
			}
		}
		
		super.OnAction(comp, action, pParentStorage, traverseStorageIndex);
	}
}