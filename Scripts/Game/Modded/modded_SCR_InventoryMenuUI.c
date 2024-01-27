modded class SCR_RadioComponent
{
	ERadioType GetRadioType()
	{
		return m_iRadioType;
	}	
};

modded class SCR_InventoryStorageBaseUI
{
	void SetCloseButtonVisibility(bool value = false)
	{
		if(!m_wCloseStorageButton)
			return;
		
		m_wCloseStorageButton.SetVisible(value);
	}
}
modded class SCR_InventoryMenuUI
{
	private ERadioType m_SelectedRadio;
	
	override void NavigationBarUpdate()
	{
		super.NavigationBarUpdate();
		
		if(!m_pFocusedSlotUI || isLootManager)
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
	
	private bool isLootManager = false;
	private SCR_TW_PlayerCrateComponent playerCrate;
	
	void SetOpenStorageToLootCrateContents(SCR_TW_PlayerCrateComponent playerCrate)
	{
		this.playerCrate = playerCrate;			
		isLootManager = true;
				
		GetInventoryStorageManager().SetStorageToOpen(playerCrate.GetOwner());
		SetOpenStorage();
	}
	
	override void Action_CloseInventory()
	{
		super.Action_CloseInventory();
		
		// We only want to do this other stuff if the menu is considered "loot manager".
		if(!isLootManager)
			return;
		
		if(!playerCrate)
		{
			Print("TrainWreck: Player crate is null. Unable to save contents", LogLevel.ERROR);
			return;
		}
		
		SCR_PlayerController controller = SCR_PlayerController.Cast(GetGame().GetPlayerController());
		controller.OnLootManagerClosed();
	}
	
	override void OnAction( SCR_InputButtonComponent comp, string action, SCR_InventoryStorageBaseUI pParentStorage = null, int traverseStorageIndex = -1 )
	{
		switch(action)
		{
			case "TW_OpenExtractionMenu":
			{
				MenuBase menuBase = GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.TW_CallExtractionMenu);
				TW_UI_ExtractionDisplay menu = TW_UI_ExtractionDisplay.Cast(menuBase);
				menu.SetRadioType(m_SelectedRadio);	
				PrintFormat("TrainWreck: Radio Type: %1", m_SelectedRadio);
				return;
			}
		}
		
		super.OnAction(comp, action, pParentStorage, traverseStorageIndex);
	}
}