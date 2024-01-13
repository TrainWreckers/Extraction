enum TW_ExtractionFrameworkComparisonOperator
{
	LESS_THAN,
	LESS_OR_EQUAL,
	GREATER_THEN,
	GREATER_OR_EQUAL,
	EQUAL,
	NOT_EQUAL
};

[BaseContainerProps()]
class TW_ExtractionFrameworkLogicInputBase
{
	[Attribute("", UIWidgets.Auto, category: "Input")]
	protected ref TW_ExtractionFrameworkActionInputBase m_ActionInput;	
	
	[Attribute("0", UIWidgets.ComboBox, "Operator", "", ParamEnumArray.FromEnum(TW_ExtractionFrameworkComparisonOperator))]
	protected TW_ExtractionFrameworkComparisonOperator m_Operator;
	
	void Init(TW_ExtractionFrameworkActionInputBase input)
	{
		m_ActionInput = input;
	}
	
	bool Evaluate();
};

//------------------------------------------------------------------------------------------------
[BaseContainerProps(), SCR_ContainerActionTitle()]
class TW_ExtractionFrameworkActionInputBase
{
	[Attribute()]
	protected TW_ExtractionFrameworkLogicInputBase _logic;
		
	void Init(TW_ExtractionFrameworkLogicInputBase input)
	{
		_logic = input;
	}
};	

class TW_ExtractionFrameworkActionInput<Class T> : TW_ExtractionFrameworkActionInputBase
{
	T m_Value;
	
	//------------------------------------------------------------------------------------------------
	T GetValue()
	{
		return m_Value;	
	}
	
	//------------------------------------------------------------------------------------------------
	void TW_ExtractionFrameworkParam(T value)
	{
		m_Value = value;
	}
};