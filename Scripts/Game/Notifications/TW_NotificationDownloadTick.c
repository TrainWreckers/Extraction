/*!
Notification Player with download increment with total number collected so far
Displays a notification: %1 = Total reward collected
SCR_NotificationData: m_iParam1 = TotalReward
*/
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(ENotification, "m_NotificationKey")]
class SCR_NotificationDownloadtick : SCR_NotificationDisplayData
{		
	override string GetText(SCR_NotificationData data)
	{
		int totalCollected;
		data.GetParams(totalCollected);
		return string.Format(super.GetText(data), totalCollected);
	}	
};

/*!
Notification Player when player leaves download area. Informing them of total players left
Displays a notification: %1 = Number of players left
SCR_NotificationData: m_iParam1 = PlayerCount
*/
[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(ENotification, "m_NotificationKey")]
class SCR_NotificationDownloadPlayerLeft : SCR_NotificationDisplayData
{		
	override string GetText(SCR_NotificationData data)
	{
		int totalPlayers;
		data.GetParams(totalPlayers);
		return string.Format(super.GetText(data), totalPlayers);
	}	
};