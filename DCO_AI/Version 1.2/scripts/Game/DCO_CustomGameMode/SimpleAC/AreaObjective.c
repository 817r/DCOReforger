class DCO_ObjectiveAreaClass : GenericEntityClass
{
}

class DCO_ObjectiveArea : GenericEntity
{
	[Attribute("", UIWidgets.EditBox, "Area Name", category: "Area Operation")]
	protected string m_sAreaName;

	[Attribute("0 0 0", UIWidgets.EditBox, "Center of the objective in local space.", category: "Area Operation", params: "inf inf 0 purposeCoords spaceEntity")]
	protected vector m_vObjectiveCenter;

	[Attribute("2", UIWidgets.Slider, "Duration in seconds before each point is awarded.", params: "0.1 100.0 0.1", category: "Area Operation")]
	protected float m_fTickRate;

	[Attribute("1", UIWidgets.Slider, "Point awarded per tick.", params: "1 1000 1", category: "Area Operation")]
	protected int m_iScorePerTick;

	[Attribute("0", UIWidgets.Slider, "Point awarded per tick when contested.", params: "0 1000 1", category: "Area Operation")]
	protected int m_iScorePerTickContested;

	[Attribute("0.5", UIWidgets.Slider, "Percent of players needed (compared to max players) to start contesting.", params: "0 1 0.01", category: "Area Operation")]
	protected float m_fContestingRatio;
	
	[Attribute("100", UIWidgets.Slider, "Point To Own", params: "20 1000 1", category: "Area Operation")]
	protected int m_iPointToOwn;

	//! Elapsed time
	protected float m_fTickTime;

	//! The faction that currently attacking/contesting this point in relation to the owner faction.
	protected Faction m_pContestingFaction;
	
	protected Faction m_OwnerFaction;

	protected override void EOnInit(IEntity owner)
	{
		super.EOnInit(owner);

		// Supress messages out of playmode, order of things is not quite guaranteed here
		if (!GetGame().InPlayMode())
			return;

		// Register self in manager
		DCO_War_GameMode parentManager = DCO_War_GameMode.GetAreaManager();
		if (!parentManager)
		{
			Print("CaptureAndHoldArea cannot find DCO_War_GameMode! Functionality might be limited!", LogLevel.WARNING);
			return;
		}

		parentManager.RegisterArea(this);
	}

	protected override void EOnFrame(IEntity owner, float timeSlice)
	{
		super.EOnFrame(owner, timeSlice);

		m_fTickTime += timeSlice;
		while (m_fTickTime >= m_fTickRate)
		{
			m_fTickTime -= m_fTickRate;
			// ON TICK EVENT
		}
	}

	protected void ~DCO_ObjectiveArea()
	{
		// Far from ideal, OnDelete would be better

		// Register self in manager
		DCO_War_GameMode parentManager = DCO_War_GameMode.GetAreaManager();
		if (!parentManager)
			return;

		parentManager.UnregisterArea(this);
	}
}
