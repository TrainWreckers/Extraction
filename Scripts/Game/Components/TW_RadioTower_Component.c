[ComponentEditorProps(category: "GameScripted/TrainWreck/Components", description: "Registers a radio tower")]
class TW_RadioTower_ComponentClass: ScriptComponentClass
{
};

class TW_RadioTower_Component : ScriptComponent
{
	ref ScriptInvoker OnPowerOnInvoker = new ScriptInvoker();	
	private bool _isPoweredOn = false;
	
	//! Indicates this tower can be turned on
	private bool _isActive = true;
	
	bool IsPoweredOn() { return _isPoweredOn; }
	bool IsTowerActive() { return _isActive; }
	
	void SetActive(bool value)
	{
		_isActive = value;
	}
	
	override void OnPostInit(IEntity owner)
	{
		if(!TW_Global.IsInRuntime())
			return;
		
		SCR_TW_ExtractionHandler.GetInstance().RegisterRadioTower(this);
	}
	
	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	void RpcAsk_TurnOn()
	{
		if(OnPowerOnInvoker)
			OnPowerOnInvoker.Invoke();
	}
	
	void TurnOn()
	{
		if(!TW_Global.IsServer(GetOwner()))
			Rpc(RpcAsk_TurnOn);
		else
			RpcAsk_TurnOn();
	}
};