// Inspiration / Source from Spyke 
class TW_GlobalRpl
{
	protected static IEntity m_MainRplEntity;
	
	static void SetMainLocalRplEntity(IEntity entity)
	{
		m_MainRplEntity = entity;
	}
	
	static IEntity GetMainLocalRplEntity()
	{
		return m_MainRplEntity;
	}
	
};