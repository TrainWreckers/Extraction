[EntityEditorProps(category: "GameScripted/TrainWreck", description: "Vehicle Spawn Point")]
class SCR_TW_VehicleSpawnClass : ScriptComponentClass {};

class SCR_TW_VehicleSpawn : ScriptComponent 
{
	[Attribute("25", UIWidgets.Slider, params: "0, 100, 1", category: "Spawn", desc: "Chance for vehicle to spawn on this point")]
	protected int m_ChanceToSpawn;
	
	[Attribute("", UIWidgets.ResourcePickerThumbnail, category: "Spawn", desc: "Prefabs that may spawn on this location", params: "et")]
	protected ref array<ResourceName> m_VehiclePrefabs;
	
	[Attribute("0.2", UIWidgets.Slider, category: "Damage", desc: "Minimum amount of damage to apply (0 means none)", params: "0.01 1 0.01")]
	protected float m_MinimumDamage;
	
	[Attribute("0.75", UIWidgets.Slider, category: "Damage", desc: "Maximum amount of damage to apply to vehicle (1 means fully damaged)", params: "0.01 1 0.01")]
	protected float m_MaximumDamage;
	
	[Attribute("0.25", UIWidgets.Slider, category: "Fuel", desc: "Minimum amount of fuel to start with", params: "0.01 1 0.01")]
	protected float m_MinimumFuel;
	
	[Attribute("0.5", UIWidgets.Slider, category: "Fuel", desc: "Maximum amount of fuel to start with", params: "0.01 1 0.01")]
	protected float m_MaximumFuel;
	
	private ref RandomGenerator random = new RandomGenerator();
	
	override void OnPostInit(IEntity owner)
	{
		// Must be in gamemode (otherwise it'll spawn in the workbench)
		if(!GetGame().InPlayMode())
			return;
		
		// Must have something available for spawn
		if(m_VehiclePrefabs.IsEmpty())
			return;
		
		// Must be from an authoritative source
		RplComponent rpl = RplComponent.Cast(owner.FindComponent(RplComponent));
		if(!rpl.IsMaster() && rpl.Role() != RplRole.Authority)
			return;
		
		if(random.RandIntInclusive(0,100) >= m_ChanceToSpawn)
			return;

		ResourceName prefabName = m_VehiclePrefabs.GetRandomElement();
		Resource prefabResource = Resource.Load(prefabName);
			
		if(!prefabResource.IsValid())
			return;
			
		EntitySpawnParams params = EntitySpawnParams();
		owner.GetTransform(params.Transform);
		params.TransformMode = ETransformMode.WORLD;
			
		IEntity vehicle = GetGame().SpawnEntityPrefab(prefabResource, GetGame().GetWorld(), params);
		
		if(!vehicle)
		{
			Print(string.Format("TrainWreck: Was unable to spawn vehicle %1", prefabName), LogLevel.ERROR);
			return;
		}
		
		DamageManagerComponent damageManager = DamageManagerComponent.Cast(vehicle.FindComponent(DamageManagerComponent));
		
		if(!damageManager)
		{
			Print(string.Format("TrainWreck: Was unable to find a DamageManagerComponent on %1", prefabName), LogLevel.ERROR);
			return;
		}
		
		ref array<HitZone> zones = {};
		damageManager.GetAllHitZones(zones);
		
		foreach(HitZone zone : zones)
		{
			float max = zone.GetMaxHealth();
			float damagePercent = random.RandFloatXY(m_MinimumDamage, m_MaximumDamage);
			float damage = max * damagePercent;
			zone.HandleDamage(damage, EDamageType.KINETIC, vehicle);		
		}			
		
		SCR_FuelManagerComponent fuelManager = SCR_FuelManagerComponent.Cast(vehicle.FindComponent(SCR_FuelManagerComponent));
		
		if(!fuelManager)
		{
			Print(string.Format("TrainWreck: Fuel Manager component not found on vehicle: %1", prefabName), LogLevel.ERROR);
			return;
		}
		
		float fuelLevel = random.RandFloatXY(m_MinimumFuel, m_MaximumFuel);
		fuelManager.SetTotalFuelPercentage(fuelLevel);
	}
	
	vector GetForwardVec()
	{
		return GetOwner().GetTransformAxis(0).Normalized();
	}
	
	#ifdef WORKBENCH
	
	[Attribute(defvalue: "1", desc: "Show the debug shapes in Workbench", category: "Debug")]
	protected bool m_ShowDebugShapesInWorkbench;
	protected WorldEditorAPI m_API;
	protected IEntity m_PreviewEntity;
	protected ref Shape m_DebugShape;
	protected int m_DebugShapeColor = Color.CYAN;
	
	protected void DrawDebugShape()
	{
		if(!m_ShowDebugShapesInWorkbench)
			return;
		
		int shapeFlags = ShapeFlags.DOUBLESIDE;
		
		vector origin = GetOwner().GetOrigin();
		
		vector start = Vector(0,1,-5);
		vector end = Vector(0,1,5);		
		
		m_DebugShape = Shape.CreateArrow(start, end, 3, m_DebugShapeColor, shapeFlags);
		vector globalTransform[4];
		GetOwner().GetTransform(globalTransform);
		m_DebugShape.SetMatrix(globalTransform);
	}
	
	override void EOnFrame(IEntity owner, float timeSlice)
	{
		if(m_ShowDebugShapesInWorkbench && m_DebugShape)
		{
			vector transform[4];
		    GetOwner().GetTransform(transform);
		    m_DebugShape.SetMatrix(transform);
		}
	}
	
	override void _WB_AfterWorldUpdate(IEntity owner, float timeSlice)
	{
		DrawDebugShape();
	}
	
	override bool _WB_OnKeyChanged(IEntity owner, BaseContainer src, string key, BaseContainerList ownerContainers, IEntity parent)
	{
		if(key == "m_ShowDebugShapesInWorkbench")
			DrawDebugShape();
		
		return true;
	}
	
	#endif		
	
};