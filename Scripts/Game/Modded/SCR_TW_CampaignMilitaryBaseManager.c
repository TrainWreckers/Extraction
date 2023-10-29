modded class SCR_CampaignMilitaryBaseManager
{
	int GetCampaignBases(notnull array<SCR_CampaignMilitaryBaseComponent> bases)
	{
		int count = 0;
		
		foreach(SCR_CampaignMilitaryBaseComponent base : m_aBases)
		{
			bases.Insert(base);
			count++;
		}
		
		return count;
	}
	
	SCR_CampaignMilitaryBaseComponent FindClosestBaseOfFaction(FactionKey faction, vector position)
	{
		SCR_CampaignMilitaryBaseComponent closestBase;
		float closestBaseDistance = float.MAX;

		foreach (SCR_CampaignMilitaryBaseComponent base : m_aBases)
		{
			if (!base.IsInitialized())
				continue;
			
			if(base.GetFaction().GetFactionKey() != faction)
				continue;
			
			float distance = vector.DistanceSq(base.GetOwner().GetOrigin(), position);

			if (distance < closestBaseDistance)
			{
				closestBaseDistance = distance;
				closestBase = base;
			}
		}

		return closestBase;
	}
	
	SCR_CampaignMilitaryBaseComponent FindClosestBaseNotFaction(FactionKey notFaction, vector position)
	{
		SCR_CampaignMilitaryBaseComponent closestBase;
		float closestBaseDistance = float.MAX;

		foreach (SCR_CampaignMilitaryBaseComponent base : m_aBases)
		{
			if (!base.IsInitialized())
				continue;
			
			if(base.GetFaction().GetFactionKey() == notFaction)
				continue;
			
			float distance = vector.DistanceSq(base.GetOwner().GetOrigin(), position);

			if (distance < closestBaseDistance)
			{
				closestBaseDistance = distance;
				closestBase = base;
			}
		}

		return closestBase;
	}
};