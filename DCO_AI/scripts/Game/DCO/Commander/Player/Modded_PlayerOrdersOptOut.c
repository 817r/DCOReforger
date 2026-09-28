modded class SCR_AIGroup
{
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

	void DCO_SendRadio(string title, string text, int kind)
	{
		Rpc(RPC_DCO_Radio, title, text, kind);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DCO_Radio(string title, string text, int kind)
	{
		DCO_PlayerComms.ShowRadio(title, text, kind);
	}

	void DCO_SendRadioKey(string title, string key, int variant, array<string> params, int kind)
	{
		Rpc(RPC_DCO_RadioKey, title, key, variant, params, kind);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DCO_RadioKey(string title, string key, int variant, array<string> params, int kind)
	{
		DCO_PlayerComms.ShowRadio(title, DCO_Radio.Resolve(key, variant, params), kind);
	}

	void DCO_SendContact(string callsign, string text, bool radio)
	{
		Rpc(RPC_DCO_Contact, callsign, text, radio);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DCO_Contact(string callsign, string text, bool radio)
	{
		DCO_ContactToast.Show(callsign, text, radio);
	}

	void DCO_SendOverlay(array<float> groups, array<float> objectives)
	{
		Rpc(RPC_DCO_Overlay, groups, objectives);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Owner)]
	protected void RPC_DCO_Overlay(array<float> groups, array<float> objectives)
	{
		DCO_MapOverlay.SetData(groups, objectives);
	}

	void DCO_RequestOrders(bool decline, string commanderUID)
	{
		Rpc(RPC_DCO_Orders, decline, commanderUID);
	}

	[RplRpc(RplChannel.Reliable, RplRcver.Server)]
	protected void RPC_DCO_Orders(bool decline, string commanderUID)
	{
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
	protected static const string DCO_ORDERS_ACTION = "MenuLockGroup";
	protected SCR_InputButtonComponent m_DCO_OrdersButton;
	protected bool m_bDCO_Pending;
	protected bool m_bDCO_PendingDecline;
	protected string m_sDCO_PendingUID;

	override void OnTabCreate(Widget menuRoot, ResourceName buttonsLayout, int index)
	{
		super.OnTabCreate(menuRoot, buttonsLayout, index);
		if (!m_PlayerGroupController)
			return;

		m_DCO_OrdersButton = CreateNavigationButton(DCO_ORDERS_ACTION, "Commander Orders", true);
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
