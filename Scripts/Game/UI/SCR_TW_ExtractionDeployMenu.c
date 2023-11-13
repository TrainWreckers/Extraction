modded enum ChimeraMenuPreset
{
	TW_ExtractionMenu
}

/*!
	Base Deploy menu class for the extraction gamemode
*/
class SCR_TW_ExtractionDeployMenuBase : ChimeraMenuBase
{
	protected SCR_InputButtonComponent m_PauseButton;
	protected SCR_InputButtonComponent m_GameMasterButton;
	protected SCR_InputButtonComponent m_ChatButton;
	
	protected SCR_ChatPanel m_ChatPanel
	protected SCR_MapEntity m_MapEntity;
	
	protected static ref OnDeployMenuOpenInvoker s_OnMenuOpen;
	
	override void OnMenuOpen()
	{
		if(!s_OnMenuOpen)
			s_OnMenuOpen.Invoke();
		
		m_GameMasterButton = SCR_InputButtonComponent.GetInputButtonComponent("Editor", GetRootWidget());
		if(m_GameMasterButton)
		{
			SCR_EditorManagerEntity editor = SCR_EditorManagerEntity.GetInstance();
			
			if(editor)
			{
				m_GameMasterButton.SetVisible(!editor.IsLimitedInstance());
				editor.GetOnLimitedChange().Insert(OnEditorLimitedChanged);
			}
		}
		
		super.OnMenuOpen();
	}
	
	override void OnMenuOpened()
	{
		// Mute sounds 
		// If menu is opened before loading screen is closed, wait for closing 
		if(ArmaReforgerLoadingAnim.IsOpen())
			ArmaReforgerLoadingAnim.m_onExitLoadingScreen.Insert(MuteSounds);
		else
			MuteSounds();
	}
	
	override void OnMenuClose()
	{
		MuteSounds(false);
		if(m_MapEntity && m_MapEntity.IsOpen())
			m_MapEntity.CloseMap();
		
		super.OnMenuClose();
	}
	
	override void OnMenuHide()
	{
		MuteSounds(false);
		if(m_MapEntity && m_MapEntity.IsOpen())
			m_MapEntity.CloseMap();
	}
	
	protected void OpenPlayerList()
	{
		GetGame().GetMenuManager().OpenDialog(ChimeraMenuPreset.PlayerListMenu);
	}
	
	void MuteSounds(bool mute = true)
	{
		if(!IsOpen())
			return;
		
		AudioSystem.SetMasterVolume(AudioSystem.SFX, !mute);
		AudioSystem.SetMasterVolume(AudioSystem.VoiceChat, !mute);
		AudioSystem.SetMasterVolume(AudioSystem.Dialog, !mute);
		
		ArmaReforgerLoadingAnim.m_onExitLoadingScreen.Remove(MuteSounds);
	}
	
	protected void OnEditorLimitedChanged(bool limited)
	{
		m_GameMasterButton.SetVisible(!limited);
	}
	
	static OnDeployMenuOpenInvoker SGetOnMenuOpen()
	{
		if(!s_OnMenuOpen)
			s_OnMenuOpen = new OnDeployMenuOpenInvoker();
		
		return s_OnMenuOpen;
	}
};

//! Main deploy menu with the map present 
class SCR_TW_ExtractionDeployMenu : SCR_TW_ExtractionDeployMenuBase
{
	protected SCR_TW_ExtractionDeployMenuHandler m_MenuHandler;
	protected SCR_GroupRequestUIComponent m_GroupRequestUIHandler;
	protected SCR_SpawnPointRequestUIComponent m_SpawnPointRequestUIHandler;
	
	protected ref MapConfiguration m_MapConfigDeploy = new MapConfiguration();
	
	protected SCR_BaseGameMode m_GameMode;
	protected SCR_RespawnComponent m_SpawnRequestManager;
	protected RplId m_SelectedSpawnPointId = RplId.Invalid();
	
	protected Widget m_wLoadingSpinner;
	protected SCR_LoadingSpinner m_LoadingSpinner;
	
	protected FactionManager m_FactionManager;
	protected SCR_PlayerFactionAffiliationComponent m_PlayerFAC;
	
	protected Widget m_wRespawnButton;
	protected SCR_DeployButton m_RespawnButton;
	
	protected Widget m_wMenuFrame;
	
	protected SCR_RespawnTimerComponent m_ActiveRespawnTimer;
	protected SCR_RespawnTimerComponent m_PlayerRespawnTimer;
	protected SCR_TimedSpawnPointComponent m_TimedSpawnPointTimer;
	protected int m_PreviousTime = 0;
	protected bool m_RespawnRequested = false;
	protected bool m_UseSuppliesEnabled;
	protected SCR_RespawnSystemComponent m_RespawnSystemComp;
	protected SCR_RespawnComponent m_RespawnComponent;
	
	protected int m_PlayerId;
	protected bool m_MapContextAllowed = true;
	protected bool m_CanREspawnAtPoint;
	
	protected SCR_UIInfoSpawnRequestResult m_UIInfoSpawnRequestResult;
	protected bool m_DisplayTime;
	
	protected float m_CurrentCanSpawnUpdateTime;
	protected const float CHECK_CAN_SPAWN_SPAWNPOINT_TIME = 1;
	protected const string FALLBACK_DEPLOY_STRING = "#AR-ButtonSelectDeploy";
	
	protected SCR_InputButtonComponent m_GroupOpenButton;
	
	protected float m_CurrentDeployTimeout;
	protected const float DEPLOY_TIME_OUT = 0.5;
	
	//! Sets map context active based on whether or not any of the selectors are focused with a gamepad 
	void AllowMapContext(bool allow)
	{
		m_MapContextAllowed = allow;	
	}
	
	static SCR_TW_ExtractionDeployMenu GetDeployMenu()
	{
		return SCR_TW_ExtractionDeployMenu.Cast(GetGame().GetMenuManager().FindMenuByPreset(ChimeraMenuPreset.TW_ExtractionMenu));
	}
	
	static void CloseDeployMenu()
	{
		GetGame().GetMenuManager().CloseMenuByPreset(ChimeraMenuPreset.TW_ExtractionMenu);
	}
	
	static SCR_TW_ExtractionDeployMenu OpenDeployMenu()
	{
		if(!GetDeployMenu())
			GetGame().GetMenuManager().OpenMenu(ChimeraMenuPreset.TW_ExtractionMenu);
		
		return GetDeployMenu();
	}
	
	protected void OpenGroupMenu()
	{
		GetGame().GetMenuManager().OpenDialog(ChimeraMenuPreset.GroupMenu);		
	}
	
	protected void UpdateRespawnButton()
	{
		int remainingTime = -1;
		
		if(m_ActiveRespawnTimer)
			remainingTime = m_ActiveRespawnTimer.GetPlayerRemainingTime(m_PlayerId);
		
		bool hasGroup = true;
		if(m_GroupRequestUIHandler && m_GroupRequestUIHandler.IsEnabled())
			hasGroup = m_GroupRequestUIHandler.GetPlayerGroup();
		
		m_RespawnButton.SetEnabled(!m_RespawnRequested && remainingTime <= 0 && m_CurrentDeployTimeout <= 0 && m_CanREspawnAtPoint && hasGroup);
	}
	
	protected void OnRespawnResponse(SCR_SpawnRequestComponent requestComponent, SCR_ESpawnResult response)
	{
		GetRootWidget().SetEnabled(true);
		m_wLoadingSpinner.SetVisible(false);
		if(response != SCR_ESpawnResult.OK)
		{
			m_RespawnButton.SetEnabled(true);
			m_RespawnButton.ShowLoading(true);
		}
		
		m_RespawnRequested = false;
	}
	
	protected void OnRespawnRequest(SCR_SpawnRequestComponent requestComponent)
	{
		m_RespawnRequested = true;
		GetRootWidget().SetEnabled(false);
		m_RespawnButton.SetEnabled(false);
		m_RespawnButton.ShowLoading(false);
		m_wLoadingSpinner.SetVisible(false);
		
		UpdateRespawnButton();
	}
	
	protected void OnPauseMenu()
	{
		GetGame().OpenPauseMenu(false, true);
	}
	
	protected void OnChatToggle()
	{
		if(!m_ChatPanel)
			return;
		
		SCR_ChatPanelManager.GetInstance().ToggleChatPanel(m_ChatPanel);
	}
	
	protected void OnPlayerFactionSet(Faction assignedFaction)
	{
		if(!assignedFaction)
			return;
		
		if(m_GroupRequestUIHandler)
			m_GroupRequestUIHandler.ShowAvailableGroups(assignedFaction);
		
		if(m_SpawnPointRequestUIHandler)
			m_SpawnPointRequestUIHandler.ShowAvailableSpawnPoints(assignedFaction);
	}
	
	protected void OnPlayerFactionResponse(SCR_PlayerFactionAffiliationComponent fac, int factionIndex, bool response)
	{
		if(response)
		{
			Faction assignedFaction = m_FactionManager.GetFactionByIndex(factionIndex);
			OnPlayerFactionSet(assignedFaction);
		}
		
		m_wLoadingSpinner.SetVisible(false);
	}
	
	protected void OnPlayerFactionRequest(SCR_PlayerFactionAffiliationComponent component, int factionIndex)
	{
		m_wLoadingSpinner.SetVisible(true);
	}
	
	//! Sends a respawn request based on assigned loadout and selected spawn point 
	protected void RequestRespawn()
	{
		UpdateRespawnButton();
		
		if(!m_RespawnButton.IsEnabled())
			return;
		
		if(!m_SelectedSpawnPointId.IsValid())
		{
			Debug.Error("Selected SpawnPointId is invalid!");
			return;
		}
		
		m_CurrentDeployTimeout = DEPLOY_TIME_OUT;
		SCR_SpawnPointSpawnData respawnData = new SCR_SpawnPointSpawnData(ResourceName.Empty, m_SelectedSpawnPointId);
		m_SpawnRequestManager.RequestSpawn(respawnData);
	}
};
