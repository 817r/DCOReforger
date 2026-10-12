modded class SCR_AIGroup
{
	override void OnWaypointAdded(AIWaypoint wp)
	{
		super.OnWaypointAdded(wp);
		DCO_Battle.OnWaypoint(this, wp);
	}

	[RplProp()]
	protected bool m_bDCO_DeclineOrders = true;

	[RplProp()]
	protected string m_sDCO_Commander;

	[RplProp()]
	protected int m_iDCO_Role = -1;

	[RplProp()]
	protected int m_iDCO_Caps;

	[RplProp()]
	protected int m_iDCO_PlayerRoles = 15;

	[RplProp()]
	protected int m_iDCO_LODInfo = -1;

	int DCO_GetLODInfo()
	{
		return m_iDCO_LODInfo;
	}

	void DCO_SetLODInfo(int info)
	{
		if (info == m_iDCO_LODInfo)
			return;
		m_iDCO_LODInfo = info;
		Replication.BumpMe();
	}

	int DCO_GetPlayerRoles()
	{
		return m_iDCO_PlayerRoles;
	}

	void DCO_SetPlayerRoles(int mask)
	{
		if (mask == m_iDCO_PlayerRoles)
			return;
		m_iDCO_PlayerRoles = mask;
		Replication.BumpMe();
	}

	int DCO_GetRole()
	{
		return m_iDCO_Role;
	}

	int DCO_GetCaps()
	{
		return m_iDCO_Caps;
	}

	void DCO_SetRoleInfo(int role, int caps)
	{
		if (role == m_iDCO_Role && caps == m_iDCO_Caps)
			return;
		m_iDCO_Role = role;
		m_iDCO_Caps = caps;
		Replication.BumpMe();
	}

	bool DCO_IsDecliningOrders()
	{
		return m_bDCO_DeclineOrders;
	}

	string DCO_GetOrdersCommanderUID()
	{
		return m_sDCO_Commander;
	}

	void DCO_SetOrders(bool decline, string commanderUID)
	{
		m_bDCO_DeclineOrders = decline;
		m_sDCO_Commander = commanderUID;
		Replication.BumpMe();
	}

	AICommander_BaseComponent DCO_GetOrdersCommander()
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		Faction f = GetFaction();
		if (!mgr || !f)
			return null;

		AICommander_BaseComponent cmd = mgr.FindCommanderByUID(m_sDCO_Commander);
		if (cmd && cmd.GetCommanderFactionKey() == f.GetFactionKey())
			return cmd;
		return DCO_PlayerContactReports.FindCommander(f);
	}
}

modded class SCR_PlayerControllerGroupComponent
{
	[RplProp(onRplName: "DCO_OnServerShareChanged")]
	protected bool m_bDCO_ServerShare = true;

	bool DCO_GetServerShare()
	{
		return m_bDCO_ServerShare;
	}

	void DCO_SetServerShare(bool share)
	{
		if (share == m_bDCO_ServerShare)
			return;
		m_bDCO_ServerShare = share;
		Replication.BumpMe();
		DCO_OnServerShareChanged();
	}

	protected void DCO_OnServerShareChanged()
	{
		DCO_SettingsSubMenu.OnServerShareChanged();
	}

	static bool DCO_LocalServerShare()
	{
		SCR_PlayerControllerGroupComponent comp = GetLocalPlayerControllerGroupComponent();
		return !comp || comp.DCO_GetServerShare();
	}

	[RplProp(onRplName: "DCO_OnServerShareChanged")]
	protected int m_iDCO_BriefCap = 2;

	[RplProp(onRplName: "DCO_OnServerShareChanged")]
	protected int m_iDCO_BriefMarkers;

	void DCO_SetBriefing(int cap, int markers)
	{
		if (cap == m_iDCO_BriefCap && markers == m_iDCO_BriefMarkers)
			return;
		m_iDCO_BriefCap = cap;
		m_iDCO_BriefMarkers = markers;
		Replication.BumpMe();
		DCO_OnServerShareChanged();
	}

	static int DCO_LocalBriefCap()
	{
		SCR_PlayerControllerGroupComponent comp = GetLocalPlayerControllerGroupComponent();
		if (!comp)
			return 2;
		return comp.m_iDCO_BriefCap;
	}

	static int DCO_LocalBriefMarkers()
	{
		SCR_PlayerControllerGroupComponent comp = GetLocalPlayerControllerGroupComponent();
		if (!comp)
			return 0;
		return comp.m_iDCO_BriefMarkers;
	}

	void DCO_SendBrief(int kind, int flags, string type, string obj, string task, string head, string lines, array<float> pts, vector info)
	{
		DCO_Perf.Count("rpc:brief");
		DCO_Perf.CountN("rpc:floats", pts.Count());
		Rpc(RPC_DCO_Brief, kind * 256 + flags, type, obj, task, head, lines, pts, info);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DCO_Brief(int kindFlags, string type, string obj, string task, string head, string lines, array<float> pts, vector info)
	{
		DCO_BriefUI.Receive(kindFlags / 256, kindFlags % 256, type, obj, task, head, lines, pts, info);
	}

	void DCO_SendRadio(string title, string text, int kind)
	{
		DCO_Perf.Count("rpc:radio");
		Rpc(RPC_DCO_Radio, title, text, kind);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DCO_Radio(string title, string text, int kind)
	{
		DCO_PlayerComms.ShowRadio(title, text, kind);
	}

	void DCO_SendRadioKey(string title, string key, int variant, array<string> params, int kind)
	{
		DCO_Perf.Count("rpc:radiokey");
		Rpc(RPC_DCO_RadioKey, title, key, variant, params, kind);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DCO_RadioKey(string title, string key, int variant, array<string> params, int kind)
	{
		DCO_PlayerComms.ShowRadio(title, DCO_Radio.Resolve(key, variant, params), kind);
	}

	void DCO_SendOffer(int id, int type, string callsign, string title, string sub, string desc, float x, float z, float remain_s, float total_s, bool leader)
	{
		DCO_Perf.Count("rpc:offer");
		float leaderFlag = 0;
		if (leader)
			leaderFlag = 1;
		Rpc(RPC_DCO_Offer, id, type, callsign, title, sub, desc, Vector(x, 0, z), Vector(remain_s, total_s, leaderFlag));
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DCO_Offer(int id, int type, string callsign, string title, string sub, string desc, vector pos, vector times)
	{
		DCO_OfferCard.Show(id, type, callsign, title, sub, desc, pos[0], pos[2], times[0], times[1], times[2] > 0.5);
	}

	void DCO_SendOfferClose(int id, int result)
	{
		Rpc(RPC_DCO_OfferClose, id, result);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DCO_OfferClose(int id, int result)
	{
		DCO_OfferCard.Close(id, result);
	}

	void DCO_AnswerOffer(int id, bool accept)
	{
		if (Replication.IsServer())
			RPC_DCO_OfferAnswer(id, accept);
		else
			Rpc(RPC_DCO_OfferAnswer, id, accept);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_DCO_OfferAnswer(int id, bool accept)
	{
		DCO_Perf.Count("rpc:offer_answer");
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (!groups)
			return;
		SCR_AIGroup grp = groups.FindGroup(GetGroupID());
		if (!grp || !grp.IsPlayerLeader(GetPlayerID()))
			return;
		AICommander_BaseComponent cmd = grp.DCO_GetOrdersCommander();
		if (cmd)
			cmd.GetPlayerTasking().AnswerOffer(cmd, grp, id, accept, GetPlayerID());
	}

	void DCO_SendContact(string callsign, string text, bool radio)
	{
		DCO_Perf.Count("rpc:contact");
		Rpc(RPC_DCO_Contact, callsign, text, radio);
	}

	void DCO_SendSpawnerCard(int spawnerId, int cmdId, int kind, string title, string text)
	{
		DCO_Perf.Count("rpc:spawner_card");
		Rpc(RPC_DCO_SpawnerCard, spawnerId, cmdId, kind, title, text);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DCO_SpawnerCard(int spawnerId, int cmdId, int kind, string title, string text)
	{
		DCO_SpawnerCard.Show(spawnerId, cmdId, kind, title, text);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DCO_Contact(string callsign, string text, bool radio)
	{
		DCO_ContactToast.Show(callsign, text, radio);
	}

	void DCO_SendOverlay(array<float> groups, array<float> objectives)
	{
		DCO_Perf.Count("rpc:overlay");
		DCO_Perf.CountN("rpc:floats", groups.Count() + objectives.Count());
		Rpc(RPC_DCO_Overlay, groups, objectives);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DCO_Overlay(array<float> groups, array<float> objectives)
	{
		DCO_MapOverlay.SetData(groups, objectives);
	}

	void DCO_SendTransportMarks(array<float> marks)
	{
		DCO_Perf.Count("rpc:marks");
		DCO_Perf.CountN("rpc:floats", marks.Count());
		Rpc(RPC_DCO_TransportMarks, marks);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DCO_TransportMarks(array<float> marks)
	{
		DCO_MapOverlay.SetTransport(marks);
	}

	void DCO_SendVoice(bool radio)
	{
		if (Replication.IsServer())
			RPC_DCO_Voice(radio);
		else
			Rpc(RPC_DCO_Voice, radio);
	}

	[RplRpc(RplChannel.Unreliable, RplRcver.Server)]
	protected void RPC_DCO_Voice(bool radio)
	{
		DCO_Perf.Count("rpc:voice");
		DCO_Hearing.OnVoice(GetGame().GetPlayerManager().GetPlayerControlledEntity(GetPlayerID()), radio);
	}

	void DCO_RequestOrders(bool decline, string commanderUID)
	{
		Rpc(RPC_DCO_Orders, decline, commanderUID);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_DCO_Orders(bool decline, string commanderUID)
	{
		DCO_Perf.Count("rpc:orders");
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (!groups)
			return;

		SCR_AIGroup grp = groups.FindGroup(GetGroupID());
		if (!grp || !grp.IsPlayerLeader(GetPlayerID()))
			return;

		grp.DCO_SetOrders(decline, commanderUID);
		if (decline)
			return;

		AICommander_BaseComponent cmd = grp.DCO_GetOrdersCommander();
		if (cmd)
			DCO_Radio.Group(grp.GetGroupID(), "COMMANDER", "orders_on_net", DCO_ERadioKind.INFO, DCO_Radio.P("callsign", DCO_PlayerComms.GetCallsign(grp), "cmd", cmd.GetCommanderUID()));
	}
}

modded class SCR_GroupSubMenuBase
{
	protected SCR_InputButtonComponent m_DCO_OrdersButton;
	protected bool m_bDCO_Pending;
	protected bool m_bDCO_PendingDecline;
	protected string m_sDCO_PendingUID;

	override void OnTabCreate(Widget menuRoot, ResourceName buttonsLayout, int index)
	{
		super.OnTabCreate(menuRoot, buttonsLayout, index);
		if (!m_PlayerGroupController)
			return;

		m_DCO_OrdersButton = CreateNavigationButton("MenuLockGroup", "Commander Orders", true);
		if (m_DCO_OrdersButton)
			m_DCO_OrdersButton.m_OnActivated.Insert(DCO_ToggleOrders);
	}

	override protected void UpdateGroups(SCR_PlayerControllerGroupComponent playerGroupController)
	{
		super.UpdateGroups(playerGroupController);

		SCR_AIGroup grp = DCO_GetOwnGroup();
		SetNavigationButtonVisible(m_DCO_OrdersButton, grp && playerGroupController && playerGroupController.IsPlayerLeaderOwnGroup());
		if (!grp)
			return;

		if (m_bDCO_Pending && grp.DCO_IsDecliningOrders() == m_bDCO_PendingDecline && grp.DCO_GetOrdersCommanderUID() == m_sDCO_PendingUID)
			m_bDCO_Pending = false;

		if (m_bDCO_Pending)
			DCO_SetOrdersLabel(grp, m_bDCO_PendingDecline, m_sDCO_PendingUID);
		else
			DCO_SetOrdersLabel(grp, grp.DCO_IsDecliningOrders(), grp.DCO_GetOrdersCommanderUID());
	}

	protected void DCO_ToggleOrders()
	{
		SCR_AIGroup grp = DCO_GetOwnGroup();
		if (!grp || !m_PlayerGroupController.IsPlayerLeaderOwnGroup())
			return;

		bool curDecline = grp.DCO_IsDecliningOrders();
		string curUID = grp.DCO_GetOrdersCommanderUID();
		if (m_bDCO_Pending)
		{
			curDecline = m_bDCO_PendingDecline;
			curUID = m_sDCO_PendingUID;
		}

		array<string> uids = DCO_GetCommanders(grp);
		int idx = -1;
		if (!curDecline)
			idx = uids.Find(curUID);
		idx++;

		bool decline = idx >= uids.Count();
		string uid;
		if (!decline)
			uid = uids[idx];

		m_bDCO_Pending = true;
		m_bDCO_PendingDecline = decline;
		m_sDCO_PendingUID = uid;
		m_PlayerGroupController.DCO_RequestOrders(decline, uid);
		DCO_SetOrdersLabel(grp, decline, uid);
	}

	protected void DCO_SetOrdersLabel(SCR_AIGroup grp, bool decline, string uid)
	{
		if (!m_DCO_OrdersButton)
			return;

		if (decline)
		{
			m_DCO_OrdersButton.SetLabel("Commander Orders: OFF");
			return;
		}

		array<string> uids = DCO_GetCommanders(grp);
		if (uids.Count() > 1 && uids.Contains(uid))
			m_DCO_OrdersButton.SetLabel("Commander Orders: " + uid);
		else
			m_DCO_OrdersButton.SetLabel("Commander Orders: ON");
	}

	protected array<string> DCO_GetCommanders(SCR_AIGroup grp)
	{
		array<string> uids = {};
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		Faction f = grp.GetFaction();
		if (mgr && f)
			mgr.GetRosterUIDsForFaction(f.GetFactionKey(), uids);
		return uids;
	}

	protected SCR_AIGroup DCO_GetOwnGroup()
	{
		if (!m_GroupManager || !m_PlayerGroupController)
			return null;
		return m_GroupManager.FindGroup(m_PlayerGroupController.GetGroupID());
	}
}
