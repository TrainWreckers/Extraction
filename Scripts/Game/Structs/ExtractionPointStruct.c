class ExtractionPointStruct : TW_Struct
{
	//! Site players need to extract at
	SCR_TW_ExtractionSite Site;
	
	void ExtractionPointStruct(SCR_TW_ExtractionSite site)
	{
		Site = site;
	}
};