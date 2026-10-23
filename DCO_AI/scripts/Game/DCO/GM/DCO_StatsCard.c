modded class SCR_ChimeraCharacter
{
	[RplProp()]
	protected float m_fDCO_Threat = -1;

	[RplProp()]
	protected int m_iDCO_ThreatState;

	[RplProp()]
	protected float m_fDCO_Morale;

	[RplProp()]
	protected int m_iDCO_MoraleState;

	[RplProp()]
	protected int m_iDCO_Traits = -1;

	int DCO_GetTraits()			{ return m_iDCO_Traits; }

	void DCO_SetTraits(int traits)
	{
		if (traits == m_iDCO_Traits)
			return;
		m_iDCO_Traits = traits;
		Replication.BumpMe();
	}

	float DCO_GetThreat()		{ return m_fDCO_Threat; }
	int DCO_GetThreatState()	{ return m_iDCO_ThreatState; }
	float DCO_GetMorale()		{ return m_fDCO_Morale; }
	int DCO_GetMoraleState()	{ return m_iDCO_MoraleState; }

	void DCO_SetStats(float threat, int threatState, float morale, int mState)
	{
		if (Math.AbsFloat(threat - m_fDCO_Threat) < 0.05 && Math.AbsFloat(morale - m_fDCO_Morale) < 0.05 && threatState == m_iDCO_ThreatState && mState == m_iDCO_MoraleState)
			return;
		m_fDCO_Threat = threat;
		m_iDCO_ThreatState = threatState;
		m_fDCO_Morale = morale;
		m_iDCO_MoraleState = mState;
		Replication.BumpMe();
	}
}

modded class SCR_AIGroup
{
	[RplProp()]
	protected float m_fDCO_AvgThreat = -1;

	[RplProp()]
	protected float m_fDCO_AvgMorale;

	[RplProp()]
	protected string m_sDCO_Task;

	[RplProp()]
	protected string m_sDCO_Doing;

	[RplProp()]
	protected vector m_vDCO_Dest;

	[RplProp()]
	protected int m_iDCO_LeaderTraits = -1;

	int DCO_GetLeaderTraits()	{ return m_iDCO_LeaderTraits; }

	void DCO_SetLeaderTraits(int traits)
	{
		if (traits == m_iDCO_LeaderTraits)
			return;
		m_iDCO_LeaderTraits = traits;
		Replication.BumpMe();
	}

	string DCO_GetTaskInfo()	{ return m_sDCO_Task; }
	string DCO_GetDoingInfo()	{ return m_sDCO_Doing; }
	vector DCO_GetDest()		{ return m_vDCO_Dest; }

	void DCO_SetTaskInfo(string task, string doing, vector dest)
	{
		if (task == m_sDCO_Task && doing == m_sDCO_Doing && vector.DistanceSqXZ(dest, m_vDCO_Dest) < 4)
			return;
		m_sDCO_Task = task;
		m_sDCO_Doing = doing;
		m_vDCO_Dest = dest;
		Replication.BumpMe();
	}

	float DCO_GetAvgThreat()	{ return m_fDCO_AvgThreat; }
	float DCO_GetAvgMorale()	{ return m_fDCO_AvgMorale; }

	void DCO_SetAvgStats(float threat, float morale)
	{
		if (Math.AbsFloat(threat - m_fDCO_AvgThreat) < 0.05 && Math.AbsFloat(morale - m_fDCO_AvgMorale) < 0.05)
			return;
		m_fDCO_AvgThreat = threat;
		m_fDCO_AvgMorale = morale;
		Replication.BumpMe();
	}

	void DCO_UpdateStats()
	{
		array<AIAgent> agents = {};
		GetAgents(agents);
		float threat;
		float morale;
		int n;
		foreach (AIAgent a : agents)
		{
			SCR_ChimeraCharacter ch = SCR_ChimeraCharacter.Cast(a.GetControlledEntity());
			SCR_AIUtilityComponent util = SCR_AIUtilityComponent.Cast(a.FindComponent(SCR_AIUtilityComponent));
			if (!ch || !util || !util.m_ThreatSystem || !DCO_Util.IsAlive(ch))
				continue;
			float t = util.m_ThreatSystem.GetThreatMeasure();
			float m;
			int ms;
			DCO_AIMoraleSystem mor = util.GetMoraleSystem();
			if (mor)
			{
				m = mor.GetMoraleMeasure();
				ms = mor.GetState();
			}
			ch.DCO_SetStats(t, util.m_ThreatSystem.GetState(), m, ms);
			int traits = DCO_Leadership.PackTraits(util);
			ch.DCO_SetTraits(traits);
			if (a == GetLeaderAgent())
				DCO_SetLeaderTraits(traits);
			threat += t;
			morale += m;
			n++;
		}
		if (n == 0)
			return;
		DCO_SetAvgStats(threat / n, morale / n);
	}
}

class DCO_GroupThreatTooltipDetail : DCO_GroupTooltipDetail
{
	void DCO_GroupThreatTooltipDetail()
	{
		m_sDisplayName = "Avg threat / morale";
	}

	override bool InitDetail(SCR_EditableEntityComponent entity, Widget widget)
	{
		SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(entity);
		if (!editableGroup)
			return false;
		m_Group = editableGroup.GetAIGroupComponent();
		m_Text = TextWidget.Cast(widget);
		if (!m_Group || !m_Text)
			return false;
		m_sShown = string.Empty;
		return true;
	}

	override protected string BuildText()
	{
		if (m_Group.DCO_GetAvgThreat() < 0)
			return "-";
		return m_Group.DCO_GetAvgThreat().ToString(1, 2) + " / " + m_Group.DCO_GetAvgMorale().ToString(1, 2);
	}
}

class DCO_GroupLeaderTooltipDetail : DCO_GroupTooltipDetail
{
	void DCO_GroupLeaderTooltipDetail()
	{
		m_sDisplayName = "Leader";
	}

	override protected string BuildText()
	{
		return DCO_Leadership.TraitsText(m_Group.DCO_GetLeaderTraits());
	}
}

class DCO_CharProfileTooltipDetail : SCR_EntityTooltipDetail
{
	protected SCR_ChimeraCharacter m_Char;
	protected TextWidget m_Text;
	protected string m_sShown;

	void DCO_CharProfileTooltipDetail()
	{
		m_Layout = "{93AF6A0EA7571B84}UI/layouts/Editor/Tooltips/TooltipPrefabs/TooltipPrefab_Text.layout";
		m_bShowLabel = true;
		m_sDisplayName = "Profile";
	}

	override bool NeedUpdate()
	{
		return true;
	}

	override bool InitDetail(SCR_EditableEntityComponent entity, Widget widget)
	{
		m_Char = SCR_ChimeraCharacter.Cast(entity.GetOwner());
		m_Text = TextWidget.Cast(widget);
		if (!m_Char || !m_Text || m_Char.DCO_GetTraits() < 0)
			return false;
		m_sShown = string.Empty;
		return true;
	}

	override void UpdateDetail(SCR_EditableEntityComponent entity)
	{
		if (!m_Char || !m_Text)
			return;
		string text = DCO_Leadership.TraitsText(m_Char.DCO_GetTraits());
		if (text == m_sShown)
			return;
		m_sShown = text;
		m_Text.SetText(text);
	}
}

class DCO_CommanderTooltipDetail : SCR_EntityTooltipDetail
{
	protected AICommander_BaseComponent m_Cmd;
	protected TextWidget m_Text;
	protected string m_sShown;

	void DCO_CommanderTooltipDetail()
	{
		m_Layout = "{93AF6A0EA7571B84}UI/layouts/Editor/Tooltips/TooltipPrefabs/TooltipPrefab_Text.layout";
		m_bShowLabel = true;
		m_sDisplayName = "Commander";
	}

	override bool NeedUpdate()
	{
		return true;
	}

	override bool InitDetail(SCR_EditableEntityComponent entity, Widget widget)
	{
		IEntity owner = entity.GetOwner();
		if (!owner)
			return false;
		m_Cmd = AICommander_BaseComponent.Cast(owner.FindComponent(AICommander_BaseComponent));
		m_Text = TextWidget.Cast(widget);
		if (!m_Cmd || !m_Text)
			return false;
		m_sShown = string.Empty;
		return true;
	}

	override void UpdateDetail(SCR_EditableEntityComponent entity)
	{
		if (!m_Cmd || !m_Text)
			return;
		string text = m_Cmd.DCO_GetRplStatus();
		if (text.IsEmpty())
			text = "No decision yet. The first one comes about a minute after placement.";
		if (text == m_sShown)
			return;
		m_sShown = text;
		m_Text.SetText(text);
	}
}

class DCO_ObjectiveTooltipDetail : SCR_EntityTooltipDetail
{
	protected CMD_AICommanderObjectiveComponent m_Obj;
	protected TextWidget m_Text;
	protected string m_sShown;

	void DCO_ObjectiveTooltipDetail()
	{
		m_Layout = "{93AF6A0EA7571B84}UI/layouts/Editor/Tooltips/TooltipPrefabs/TooltipPrefab_Text.layout";
		m_bShowLabel = true;
		m_sDisplayName = "Objective";
	}

	override bool NeedUpdate()
	{
		return true;
	}

	override bool InitDetail(SCR_EditableEntityComponent entity, Widget widget)
	{
		IEntity owner = entity.GetOwner();
		if (!owner)
			return false;
		m_Obj = CMD_AICommanderObjectiveComponent.Cast(owner.FindComponent(CMD_AICommanderObjectiveComponent));
		m_Text = TextWidget.Cast(widget);
		if (!m_Obj || !m_Text)
			return false;
		m_sShown = string.Empty;
		return true;
	}

	override void UpdateDetail(SCR_EditableEntityComponent entity)
	{
		if (!m_Obj || !m_Text)
			return;
		string owner = m_Obj.DCO_GetRplOwner();
		if (owner.IsEmpty())
			owner = "Neutral";
		string text = string.Format("%1 · %2% control · radius %3 m", owner, Math.Round(m_Obj.DCO_GetRplControl()), Math.Round(m_Obj.GetRadius()));
		if (text == m_sShown)
			return;
		m_sShown = text;
		m_Text.SetText(text);
	}
}

class DCO_SpawnerTooltipDetail : SCR_EntityTooltipDetail
{
	protected DCO_CommanderSpawnerComponent m_Spawner;
	protected TextWidget m_Text;
	protected string m_sShown;

	void DCO_SpawnerTooltipDetail()
	{
		m_Layout = "{93AF6A0EA7571B84}UI/layouts/Editor/Tooltips/TooltipPrefabs/TooltipPrefab_Text.layout";
		m_bShowLabel = true;
		m_sDisplayName = "Spawner";
	}

	override bool NeedUpdate()
	{
		return true;
	}

	override bool InitDetail(SCR_EditableEntityComponent entity, Widget widget)
	{
		IEntity owner = entity.GetOwner();
		if (!owner)
			return false;
		m_Spawner = DCO_CommanderSpawnerComponent.Cast(owner.FindComponent(DCO_CommanderSpawnerComponent));
		m_Text = TextWidget.Cast(widget);
		if (!m_Spawner || !m_Text)
			return false;
		m_sShown = string.Empty;
		return true;
	}

	static string Field(array<string> f, int i)
	{
		if (i < 0 || i >= f.Count() || f[i].IsEmpty())
			return "-";
		return f[i];
	}

	override void UpdateDetail(SCR_EditableEntityComponent entity)
	{
		if (!m_Spawner || !m_Text)
			return;
		array<string> f = {};
		m_Spawner.GetRplStatus().Split("\n", f, false);
		string text = "No status yet";
		if (f.Count() > 0)
			text = string.Format("%1. Feeds %2. Preset %3. Spawned %4. Next spawn %5.", Field(f, 0), Field(f, 1), Field(f, 14), Field(f, 2), Field(f, 4));
		if (text == m_sShown)
			return;
		m_sShown = text;
		m_Text.SetText(text);
	}
}

class DCO_GroupTaskTooltipDetail : DCO_GroupTooltipDetail
{
	protected bool m_bDoing;

	void DCO_GroupTaskTooltipDetail(bool doing)
	{
		m_bDoing = doing;
		if (doing)
			m_sDisplayName = "Doing";
		else
			m_sDisplayName = "Task";
	}

	override protected string BuildText()
	{
		string text = m_Group.DCO_GetTaskInfo();
		if (m_bDoing)
			text = m_Group.DCO_GetDoingInfo();
		if (text.IsEmpty())
			return "-";
		return text;
	}
}

[BaseContainerProps(), SCR_BaseContainerCustomTitleEnum(EEditableEntityState, "m_State")]
modded class SCR_BaseEditableEntityFilter
{
	protected static void DCO_DrawObjectiveRing(BaseWorld w, SCR_EditableEntityComponent entity)
	{
		if (DCO_SettingsDialogUI.DCO_IsOpen())
			return;
		if (!w || !entity || !entity.GetOwner())
			return;
		IEntity owner = entity.GetOwner();
		CMD_AICommanderObjectiveComponent obj = CMD_AICommanderObjectiveComponent.Cast(owner.FindComponent(CMD_AICommanderObjectiveComponent));
		if (!obj)
			return;
		float r = obj.GetRadius();
		if (r <= 0)
			return;
		vector c = owner.GetOrigin();
		int col = ARGB(255, 255, 170, 0);
		if (obj.DCO_GetRplOwner() == "Neutral" || obj.DCO_GetRplOwner().IsEmpty())
			col = ARGB(255, 230, 230, 230);
		vector ring[49];
		for (int i = 0; i < 49; i++)
		{
			float a = i * Math.PI2 / 48;
			float x = c[0] + Math.Cos(a) * r;
			float z = c[2] + Math.Sin(a) * r;
			ring[i] = Vector(x, w.GetSurfaceY(x, z) + 1.5, z);
		}
		Shape.CreateLines(col, ShapeFlags.ONCE | ShapeFlags.NOZBUFFER, ring, 49);
		vector pole[2];
		pole[0] = Vector(c[0], w.GetSurfaceY(c[0], c[2]), c[2]);
		pole[1] = Vector(c[0], w.GetSurfaceY(c[0], c[2]) + 15, c[2]);
		Shape.CreateLines(col, ShapeFlags.ONCE | ShapeFlags.NOZBUFFER, pole, 2);
	}

	override void OnFrameBase(float timeSlice)
	{
		super.OnFrameBase(timeSlice);
		if (m_State != EEditableEntityState.SELECTED || !m_Entities)
			return;

		BaseWorld w = GetGame().GetWorld();
		foreach (SCR_EditableEntityComponent entity : m_Entities)
		{
			DCO_DrawObjectiveRing(w, entity);
			SCR_EditableGroupComponent editableGroup = SCR_EditableGroupComponent.Cast(entity);
			if (!editableGroup || !editableGroup.GetAIGroupComponent())
				continue;

			vector dest = editableGroup.GetAIGroupComponent().DCO_GetDest();
			vector from;
			if (dest == vector.Zero || !entity.GetPos(from))
				continue;

			from[1] = w.GetSurfaceY(from[0], from[2]) + 2;
			dest[1] = w.GetSurfaceY(dest[0], dest[2]) + 2;
			Shape.CreateArrow(from, dest, 8, ARGB(255, 255, 210, 0), ShapeFlags.ONCE | ShapeFlags.NOZBUFFER);
		}
	}
}

class DCO_CharStatsTooltipDetail : SCR_EntityTooltipDetail
{
	protected SCR_ChimeraCharacter m_Char;
	protected TextWidget m_Text;
	protected string m_sShown;
	protected bool m_bMorale;

	void DCO_CharStatsTooltipDetail(bool morale)
	{
		m_Layout = "{93AF6A0EA7571B84}UI/layouts/Editor/Tooltips/TooltipPrefabs/TooltipPrefab_Text.layout";
		m_bShowLabel = true;
		m_bMorale = morale;
		if (morale)
			m_sDisplayName = "Morale";
		else
			m_sDisplayName = "Threat";
	}

	override bool NeedUpdate()
	{
		return true;
	}

	override bool InitDetail(SCR_EditableEntityComponent entity, Widget widget)
	{
		m_Char = SCR_ChimeraCharacter.Cast(entity.GetOwner());
		m_Text = TextWidget.Cast(widget);
		if (!m_Char || !m_Text || m_Char.DCO_GetThreat() < 0)
			return false;
		m_sShown = string.Empty;
		return true;
	}

	override void UpdateDetail(SCR_EditableEntityComponent entity)
	{
		if (!m_Char || !m_Text)
			return;
		string text;
		if (m_bMorale)
			text = m_Char.DCO_GetMorale().ToString(1, 2) + " " + typename.EnumToString(moraleState, m_Char.DCO_GetMoraleState());
		else
			text = m_Char.DCO_GetThreat().ToString(1, 2) + " " + typename.EnumToString(EAIThreatState, m_Char.DCO_GetThreatState());
		if (text == m_sShown)
			return;
		m_sShown = text;
		m_Text.SetText(text);
	}
}
