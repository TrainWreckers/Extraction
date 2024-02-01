[BaseContainerProps(configRoot: true)]
class TW_Category
{
	[Attribute("", UIWidgets.EditBox, category: "Info", desc: "Name of category")]
	protected string m_Name;
	
	[Attribute("", UIWidgets.EditBoxMultiline, category: "Info", desc: "Description of category")]
	protected string m_Description;
	
	[Attribute("", UIWidgets.Auto, category: "Items", desc: "Items which belong to this category")]
	protected ref array<ResourceName> m_Items;
	
	private ref set<ResourceName> _hashmap = new set<ResourceName>();
	
	bool HasPrefab(ResourceName name)
	{
		if(_hashmap.IsEmpty())
		{
			foreach(ResourceName resource : m_Items)
				_hashmap.Insert(resource);
		}
		
		return _hashmap.Contains(name);
	}
	
	ResourceName GetRandomElement() { return m_Items.GetRandomElement(); }
	
	
	private string nameList = "";
	
	string GetName() { return m_Name; }
	
	//! Return a list of names associated with this category. (Already translated)
	string GetNameList()
	{
		if(nameList != "")
			return nameList;
		
		// This is used to help prevent duplicate naming since it's a hashmap
		ref set<string> names = new set<string>();
		
		// Since we don't have a 'set' version for string joining
		ref array<string> values = {};
		
		foreach(ResourceName resource : m_Items)
		{
			string name = WidgetManager.Translate(SCR_TW_Util.GetPrefabDisplayName(resource));
			
			if(names.Contains(name))
				continue;
			names.Insert(name);
			values.Insert(name);
		}
		
		nameList = SCR_StringHelper.Join("\n", values);
		return nameList;
	}
};