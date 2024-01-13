[BaseContainerProps(configRoot: true)]
class TW_ExtractionFrameworkOutputBase {};

class TW_ExtractionFrameworkOutput<Class T> : TW_ExtractionFrameworkOutputBase
{
	ref T m_Value;
	
	T GetValue() { return m_Value; }
	
	void TW_ExtractionFrameworkOutput(T value) 
	{
		m_Value = value;
	}
};

[BaseContainerProps(configRoot: true)]
class TW_ExtractionFrameworkSteps
{
	[Attribute("", UIWidgets.Auto, params: "conf class=TW_ExtractionFrameworkStepAction")]
	ref array<ref TW_ExtractionFrameworkStepAction> m_Steps;
}

[BaseContainerProps(configRoot: true), SCR_ContainerActionTitle()]
class TW_ExtractionFrameworkStepAction
{
	protected TW_ExtractionFrameworkOutputBase m_Output;
	
	void Evaluate(TW_ExtractionFrameworkOutputBase input)
	{
	
	}
	
	TW_ExtractionFrameworkOutputBase GetOutput() { return m_Output; }
};

[BaseContainerProps(configRoot: true), SCR_ContainerActionTitle()]
class TW_GetExtractionPointAction : TW_ExtractionFrameworkStepAction
{
	override void Evaluate(TW_ExtractionFrameworkOutputBase input)
	{
		SCR_TW_ExtractionSite site = SCR_TW_ExtractionHandler.GetInstance().GetRegisteredExtractionSite();
		
		if(!site)
		{
			m_Output = null;
			return;
		}
		
		ref ExtractionPointStruct struct = new ExtractionPointStruct(site);
		ref auto value = new TW_ExtractionFrameworkOutput<ExtractionPointStruct>(struct);
		m_Output = value;
	}
};

[BaseContainerProps(configRoot: true)]
class TW_GetRandomPositionAroundPlayer : TW_ExtractionFrameworkStepAction
{
	override void Evaluate(TW_ExtractionFrameworkOutputBase input)
	{
		ref array<int> playerIds = {};
		int playerCount = GetGame().GetPlayerManager().GetPlayers(playerIds);
		
		
	}
};

enum TW_ExtractionFrameworkEntityType
{
	PLAYERS,
	AI_GROUPS,
	AI_AGENTS
};

class EntitiesNearStruct
{
	bool IsNear;
	int NearCount;
	vector Position;
	
	void EntitiesNearStruct(bool isNear, int nearCount, vector position)
	{
		IsNear = isNear;
		NearCount = nearCount;
		Position = position;
	}
};

[BaseContainerProps(configRoot: true), SCR_ContainerActionTitle()]
class TW_GetEntitiesNearAction : TW_ExtractionFrameworkStepAction 
{
	[Attribute("", UIWidgets.ComboBox, "Operator", "", ParamEnumArray.FromEnum(TW_ExtractionFrameworkComparisonOperator), category: "Distance")]
	protected TW_ExtractionFrameworkComparisonOperator m_Operator;
	
	[Attribute("100", UIWidgets.Slider, params: "1 1000 10", desc: "Distance to point", category: "Distance")]
	protected float m_Distance;		
	
	[Attribute("", UIWidgets.ComboBox, "Type", "", ParamEnumArray.FromEnum(TW_ExtractionFrameworkEntityType), category: "Condition")]
	protected TW_ExtractionFrameworkEntityType m_EntityType;
	
	[Attribute("1", UIWidgets.Slider, params: "1 50 1", desc: "Number of entities to be near point", category: "Condition")]
	protected int m_EntityCount;
	
	//! Compared to m_Entity does provided count meet comparison operation. 
	private int EvaluateCount(int count)
	{
		switch(m_Operator)
		{
			case TW_ExtractionFrameworkComparisonOperator.EQUAL:
				return count == m_EntityCount;
			case TW_ExtractionFrameworkComparisonOperator.NOT_EQUAL:
				return count != m_EntityCount;
			case TW_ExtractionFrameworkComparisonOperator.LESS_THAN:
				return count < m_EntityCount;
			case TW_ExtractionFrameworkComparisonOperator.LESS_OR_EQUAL:
				return count <= m_EntityCount;
			case TW_ExtractionFrameworkComparisonOperator.GREATER_THEN:
				return count > m_EntityCount;
			case TW_ExtractionFrameworkComparisonOperator.GREATER_OR_EQUAL:
				return count >= m_EntityCount;			
		}		
		
		return false;
	}
	
	private bool IsNear(IEntity entity, vector position)
	{
		float distance = vector.Distance(entity.GetOrigin(), position);
		
		return distance <= m_Distance;
	}
	
	private int GetEntities(vector position, out notnull array<IEntity> entities)
	{
		int count = 0;
		switch(m_EntityType)
		{
			case TW_ExtractionFrameworkEntityType.PLAYERS:
			{
				ref array<int> playerIds = {};
				GetGame().GetPlayerManager().GetPlayers(playerIds);
				foreach(int playerId : playerIds)
				{
					IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
					if(!playerEntity)
						continue;
					
					if(!IsNear(playerEntity, position))
						continue;
					
					entities.Insert(playerEntity);
					count++;
				}
				
				break;
			}
			
			case TW_ExtractionFrameworkEntityType.AI_AGENTS:
			{
				ref array<AIAgent> agents = {};
				GetGame().GetAIWorld().GetAIAgents(agents);
				
				foreach(AIAgent agent : agents)
				{
					if(!agent || !IsNear(agent, position))
						continue;
					count++;
					entities.Insert(agent);
				}
				
				break;
			}
			
			case TW_ExtractionFrameworkEntityType.AI_GROUPS:
			{
				ref array<AIAgent> agents = {};
				GetGame().GetAIWorld().GetAIAgents(agents);
				
				foreach(AIAgent agent : agents)
				{
					SCR_AIGroup group = SCR_AIGroup.Cast(agent);
					if(!group || !IsNear(group, position))
						continue;
					count++;
					entities.Insert(group);
				}
				
				break;
			}
		}
		
		return count;
	}
	
	override void Evaluate(TW_ExtractionFrameworkOutputBase input)
	{
		TW_ExtractionFrameworkOutput<ExtractionPointStruct> eps = TW_ExtractionFrameworkOutput<ExtractionPointStruct>.Cast(input);
		
		if(eps)
			ProcessExtractionPosition(eps.GetValue());
		else
			Print("TW_GetEntitiesNearAction expected a position based input", LogLevel.WARNING);
	}
	
	private void ProcessExtractionPosition(ExtractionPointStruct point)
	{
		vector origin = point.Site.GetOrigin();
		ref array<IEntity> entities = {};
		int count = GetEntities(origin, entities);
		bool valid = EvaluateCount(count);
		
		EntitiesNearStruct struct = new EntitiesNearStruct(valid, count, origin);
		TW_ExtractionFrameworkOutput<EntitiesNearStruct> output = new TW_ExtractionFrameworkOutput<EntitiesNearStruct>(struct);
		m_Output = output;
	}
};