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

[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(ENotification, "m_NotificationKey")]
class SCR_NotificationNameFormat1Arg : SCR_NotificationDisplayData
{
	override string GetText(SCR_NotificationData data)
	{
		int value1;
		data.GetParams(value1);
		return string.Format(super.GetText(data), value1);
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(ENotification, "m_NotificationKey")]
class SCR_NotificationNameFormat2Arg: SCR_NotificationDisplayData
{
	override string GetText(SCR_NotificationData data)
	{
		int value1, value2;
		data.GetParams(value1, value2);
		return string.Format(super.GetText(data), value1, value2);
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(ENotification, "m_NotificationKey")]
class SCR_NotificationNameFormat3Arg: SCR_NotificationDisplayData
{
	override string GetText(SCR_NotificationData data)
	{
		int value1, value2, value3;
		data.GetParams(value1, value2, value3);
		return string.Format(super.GetText(data), value1, value2, value3);
	}
}