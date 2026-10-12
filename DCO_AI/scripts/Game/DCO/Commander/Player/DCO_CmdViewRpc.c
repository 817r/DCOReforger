modded class SCR_PlayerControllerGroupComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		if (!System.IsConsoleApp())
			GetGame().GetCallqueue().CallLater(DCO_CmdViewUI.EnsureHooked, 500);
	}

	void DCO_CVSubscribe(bool on, int layers)
	{
		if (Replication.IsServer())
			DCO_CmdViewServer.Subscribe(GetPlayerID(), on, layers);
		else
			Rpc(RPC_DCO_CVSub, on, layers);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_DCO_CVSub(bool on, int layers)
	{
		DCO_CmdViewServer.Subscribe(GetPlayerID(), on, layers);
	}

	void DCO_CVRequestDetail(int type, string id)
	{
		if (Replication.IsServer())
			DCO_CmdViewServer.Detail(GetPlayerID(), type, id);
		else
			Rpc(RPC_DCO_CVDetailReq, type, id);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_DCO_CVDetailReq(int type, string id)
	{
		DCO_CmdViewServer.Detail(GetPlayerID(), type, id);
	}

	void DCO_CVSendSnap(array<float> f, array<string> s)
	{
		DCO_Perf.CountN("rpc:floats", f.Count());
		Rpc(RPC_DCO_CVSnap, f, s);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DCO_CVSnap(array<float> f, array<string> s)
	{
		DCO_CmdViewUI.Receive(f, s);
	}

	void DCO_CDSend(array<float> f, array<string> s)
	{
		Rpc(RPC_DCO_CD, f, s);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DCO_CD(array<float> f, array<string> s)
	{
		DCO_CmdDbg.Receive(f, s);
	}

	void DCO_CVSendDetail(int type, string id, array<string> rows)
	{
		Rpc(RPC_DCO_CVDetail, type, id, rows);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DCO_CVDetail(int type, string id, array<string> rows)
	{
		DCO_CmdViewUI.ReceiveDetail(type, id, rows);
	}
}

modded class SCR_AIGroup
{
	[RplProp()]
	protected string m_sDCO_Plan;

	[RplProp()]
	protected string m_sDCO_TRPhase;

	[RplProp()]
	protected string m_sDCO_Why;

	string DCO_GetPlanInfo()	{ return m_sDCO_Plan; }
	string DCO_GetTRPhaseInfo()	{ return m_sDCO_TRPhase; }
	string DCO_GetWhyInfo()		{ return m_sDCO_Why; }

	void DCO_SetPlanInfo(string plan, string phase, string why)
	{
		if (plan == m_sDCO_Plan && phase == m_sDCO_TRPhase && why == m_sDCO_Why)
			return;
		m_sDCO_Plan = plan;
		m_sDCO_TRPhase = phase;
		m_sDCO_Why = why;
		Replication.BumpMe();
	}
}
