/*!
	This component serves as a config for deploy menu elements.

	Set the names of widgets where request handlers are attached to in the deploy menu layout LayoutSlot
	Has to be attached at the root of the deploy menu layout
*/

class SCR_TW_ExtractionDeployMenuHandler : SCR_ScriptedWidgetComponent 
{
	[Attribute("FactionOverlay")]
	protected string m_FactionHandlerName;
	protected SCR_FactionRequestUIComponent m_FactionRequestHandler;
	
	[Attribute("GroupSelector")]
	protected string m_GroupUIHandlerName;
	protected SCR_GroupRequestUIComponent m_GroupRequestHandler;
	
	[Attribute("SpawnPointSelector")]
	protected string m_SpawnPointUIHandlerName;
	protected SCR_SpawnPointRequestUIComponent m_SpawnPointUIHandler;
	
	[Attribute("FactionPlayerList")]
	protected string m_FactionPlayerListName;
	protected SCR_FactionPlayerList m_FactionPlayerList;
	
	[Attribute("GroupPlayerList")]
	protected string m_GroupPlayerListName;
	protected SCR_GroupPlayerList m_GroupPlayerList;
	
	[Attribute(desc: "Determines which widgets should be hidden when opening the pause menu")]
	protected ref array<string> m_HiddenWidgetsOnPause;
	
	[Attribute(desc: "Determines which widgets should be disabled when opening the pause menu")]
	protected ref array<string> m_DisabledWidgetsOnPause;
	
	protected ref array<Widget> m_HiddenWidgets = {};
	protected ref array<Widget> m_DisabledWidgets = {};
	
	override void HandlerAttached(Widget w)
	{
		super.HandlerAttached(w);
		
		PauseMenuUI.m_OnPauseMenuOpened.Insert(OnPauseMenuOpened);
		PauseMenuUI.m_OnPauseMenuClosed.Insert(OnPauseMenuClosed);
		
		foreach(string name : m_HiddenWidgetsOnPause)
		{
			Widget widget = m_wRoot.FindAnyWidget(name);
			
			if(widget)
				m_HiddenWidgets.Insert(widget);
		}
		
		foreach(string name : m_DisabledWidgetsOnPause)
		{
			Widget widget = m_wRoot.FindAnyWidget(name);
			if(widget)
				m_DisabledWidgets.Insert(widget);
		}
	}
	
	override void HandlerDeattached(Widget w)
	{
		super.HandlerDeattached(w);
		
		PauseMenuUI.m_OnPauseMenuOpened.Remove(OnPauseMenuOpened);
    	PauseMenuUI.m_OnPauseMenuClosed.Remove(OnPauseMenuClosed);
	}
	
	protected void OnPauseMenuClosed()
	{
		UpdateWidgetsOnPause(true);
	}
	
	protected void OnPauseMenuOpened()
	{
		UpdateWidgetsOnPause(false);
	}
	
	protected void UpdateWidgetsOnPause(bool paused)
	{
		foreach(Widget widget : m_HiddenWidgets)
			widget.SetVisible(!paused);

		foreach(Widget widget : m_DisabledWidgets)
			widget.SetEnabled(!paused);
	}
	
	SCR_FactionRequestUIComponent GetFactionRequestHandler()
	{
		Widget tmp = m_wRoot.FindAnyWidget(m_FactionHandlerName);
		
		if(!tmp)
			return null;
		
		return SCR_FactionRequestUIComponent.Cast(tmp.FindHandler(SCR_FactionRequestUIComponent));
	}
	
	SCR_GroupRequestUIComponent GetGroupRequestHandler()
	{
		Widget tmp = m_wRoot.FindAnyWidget(m_GroupUIHandlerName);
		
		if(!tmp)
			return null;
		
		return SCR_GroupRequestUIComponent.Cast(tmp.FindHandler(SCR_GroupRequestUIComponent));
	}
	
	SCR_SpawnPointRequestUIComponent GetSpawnPointRequestHandler()
	{
		Widget tmp = m_wRoot.FindAnyWidget(m_SpawnPointUIHandlerName);
		
		if(!tmp)
			return null;
		
		return SCR_SpawnPointRequestUIComponent.Cast(tmp.FindHandler(SCR_SpawnPointRequestUIComponent));
	}
	
	SCR_FactionPlayerList GetFactionPlayerList()
	{
		Widget tmp = m_wRoot.FindAnyWidget(m_FactionPlayerListName);
		
		if(!tmp)
			return null;
		
		return SCR_FactionPlayerList.Cast(tmp.FindHandler(SCR_FactionPlayerList));
	}
	
	SCR_GroupPlayerList GetGroupPlayerList()
	{
		Widget tmp = m_wRoot.FindAnyWidget(m_GroupPlayerListName);
		
		if(!tmp)
			return null;
		
		return SCR_GroupPlayerList.Cast(tmp.FindHandler(SCR_GroupPlayerList));
	}
};