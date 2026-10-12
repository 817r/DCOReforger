enum DCO_EPlayerTaskType
{
	MOVE,
	ATTACK,
	DEFEND,
	ROLE
}

class DCO_PlayerTaskEntry
{
	SCR_Task m_Task;
	CMD_AICommanderObjectiveComponent m_Objective;
	DCO_EPlayerTaskType m_eType;
	vector m_vPos;
	bool m_bDone;

	bool m_bCounted;
	bool m_bAccepted;
	bool m_bLate;
	bool m_bSlot;
	float m_fCreated_s;
	float m_fStartDist;
	float m_fEtaDeadline_s;
	float m_fLastDesc_s;

	int m_iRole = -1;
	int m_eTactic = -1;
	vector m_vRetreat;
	bool m_bFlank;
	bool m_bArrived;
	bool m_bInVehicle;
	bool m_bOffered;
	bool m_bOfferAnswered;
	float m_fOfferAt_s;
	float m_fOfferEta;
	bool m_bPending;
	int m_iOfferId;
}

class DCO_PlayerOffer
{
	int m_iId;
	int m_iGroup;
	int m_iType;
	int m_iLeader = -1;
	float m_fQueued_s;
	float m_fSent_s;
	float m_fExpire_s;
	string m_sCallsign;
	string m_sTitle;
	string m_sSub;
	string m_sDesc;
	string m_sTactic;
	string m_sRole;
	string m_sObj;
	vector m_vPos;
	ref array<int> m_aSentTo = {};
}

class DCO_PlayerReliability
{
	int m_iFails;
	float m_fBonusUntil_s;
	float m_fCooldownUntil_s;
}

class DCO_PlayerRoleExclude
{
	CMD_AICommanderObjectiveComponent m_Obj;
	int m_iBits;
	float m_fUntil_s;
}

class DCO_PlayerTasking
{


	protected ref map<int, ref DCO_PlayerTaskEntry> m_mTasks = new map<int, ref DCO_PlayerTaskEntry>();
	protected ref map<int, ref DCO_PlayerReliability> m_mReliability = new map<int, ref DCO_PlayerReliability>();
	protected ref map<int, ref DCO_PlayerRoleExclude> m_mExclude = new map<int, ref DCO_PlayerRoleExclude>();
	protected ref map<int, float> m_mOtherCd = new map<int, float>();
	protected int m_iSeq;
	protected ref map<int, ref DCO_PlayerOffer> m_mOffer = new map<int, ref DCO_PlayerOffer>();
	protected ref map<int, ref array<ref DCO_PlayerOffer>> m_mOfferQueue = new map<int, ref array<ref DCO_PlayerOffer>>();
	protected ref map<int, float> m_mOfferGap = new map<int, float>();
	protected int m_iOfferSeq;

	static void HandleRadial(int pid, int bit, bool requestOther)
	{
		if (!Replication.IsServer())
			return;

		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (!groups)
			return;

		SCR_AIGroup grp = groups.GetPlayerGroup(pid);
		if (!grp || !grp.IsPlayerLeader(pid))
		{
			DCO_Radio.Player(pid, "COMMANDER", "only_leader", DCO_ERadioKind.WARNING);
			return;
		}

		AICommander_BaseComponent cmd;
		if (!grp.DCO_IsDecliningOrders())
			cmd = grp.DCO_GetOrdersCommander();

		if (bit != 0)
		{
			int mask = grp.DCO_GetPlayerRoles() ^ bit;
			grp.DCO_SetPlayerRoles(mask);
			bool on = (mask & bit) != 0;
			string roleKey = "role_off";
			if (on)
				roleKey = "role_on";
			DCO_Radio.Group(grp.GetGroupID(), "COMMANDER", roleKey, DCO_ERadioKind.INFO, DCO_Radio.P("role", BitKey(bit)));
			DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("player_role_toggle grp=%1 role=%2 on=%3", grp.GetGroupID(), BitLabel(bit), on));
			if (!on && cmd)
				cmd.GetPlayerTasking().OnRoleDisabled(cmd, grp, bit);
			return;
		}

		if (!cmd)
		{
			DCO_Radio.Player(pid, "COMMANDER", "no_net", DCO_ERadioKind.WARNING);
			return;
		}

		if (requestOther)
			cmd.GetPlayerTasking().RequestOther(cmd, grp);
		else
			cmd.GetPlayerTasking().AcceptTransport(cmd, grp, pid);
	}

	static string BitKey(int bit)
	{
		switch (bit)
		{
			case 1:	return "@role_assault";
			case 2:		return "@role_fix";
			case 4:	return "@role_block";
			case 8:	return "@role_recon";
		}
		return "@role_assault";
	}

	static string BitLabel(int bit)
	{
		switch (bit)
		{
			case 1:	return "Assault/Maneuver";
			case 2:		return "Fix/SBF";
			case 4:	return "Block/Anvil";
			case 8:	return "Recon/Infiltration";
		}
		return "Defend";
	}

	void Update(AICommander_BaseComponent cmd)
	{
		SCR_TaskSystem ts = SCR_TaskSystem.GetInstance();
		if (!ts || !Replication.IsServer())
			return;

		array<SCR_AIGroup> eligible = {};
		Eligible(cmd, eligible);

		float now = DCO_Util.NowS();
		array<int> seen = {};
		foreach (SCR_AIGroup grp : eligible)
		{
			int id = grp.GetGroupID();
			DCO_PlayerReliability rel = m_mReliability.Get(id);
			if (rel && now < rel.m_fCooldownUntil_s && !m_mTasks.Contains(id))
				continue;

			seen.Insert(id);
			UpdateGroup(ts, cmd, grp, id);
		}

		for (int i = m_mTasks.Count() - 1; i >= 0; i--)
		{
			int key = m_mTasks.GetKey(i);
			if (!seen.Contains(key))
			{
				DCO_PlayerTaskEntry gone = m_mTasks.GetElement(i);
				if (gone.m_eType == DCO_EPlayerTaskType.ROLE && !gone.m_bDone && gone.m_Objective)
					cmd.GetTactics().RefillRole(cmd, gone.m_Objective, key, now);
				Finish(ts, gone, cmd);
				m_mTasks.RemoveElement(i);
			}
		}
	}

	void CheckCompletion(AICommander_BaseComponent cmd)
	{
		SCR_TaskSystem ts = SCR_TaskSystem.GetInstance();
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (!ts || !groups)
			return;

		ProcessOffers(cmd);

		float now = DCO_Util.NowS();
		array<int> remove = {};
		array<CMD_AICommanderObjectiveComponent> early = {};
		foreach (int id, DCO_PlayerTaskEntry e : m_mTasks)
		{
			SCR_AIGroup grp = groups.FindGroup(id);
			if (grp && (grp.DCO_IsDecliningOrders() || grp.DCO_GetOrdersCommander() != cmd))
			{
				remove.Insert(id);
				continue;
			}

			if (e.m_bDone || !e.m_Task || !grp)
				continue;

			vector from = LeaderPos(grp);

			if (e.m_eType == DCO_EPlayerTaskType.ROLE)
				UpdateRoleArrival(grp, id, e);

			if (IsAccomplished(e, cmd, grp))
			{
				Finish(ts, e, cmd, true, e.m_eType == DCO_EPlayerTaskType.MOVE);
				RecordSuccess(id);
				continue;
			}

			bool travels = e.m_eType == DCO_EPlayerTaskType.MOVE || e.m_eType == DCO_EPlayerTaskType.ROLE;
			if (e.m_bCounted && !e.m_bLate)
			{
				float dist = vector.DistanceXZ(from, e.m_vPos);
				if (!e.m_bAccepted)
				{
					if (e.m_fStartDist - dist >= 30.0 || dist <= ArriveRadius(e))
						Accept(cmd, grp, id, e, from);
					else if (now - e.m_fCreated_s > cmd.GetPlayerAcceptTime())
					{
						DCO_Radio.Group(id, "COMMANDER", "task_no_movement", DCO_ERadioKind.WARNING, DCO_Radio.P("callsign", DCO_PlayerComms.GetCallsign(grp), "task", e.m_Task.GetTaskName()));
						DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("group %1 did not accept the task within %2 s -> released to AI", id, cmd.GetPlayerAcceptTime()));
						RecordFail(cmd, id, true);
						if (e.m_eType == DCO_EPlayerTaskType.ROLE && e.m_Objective)
							cmd.GetTactics().RefillRole(cmd, e.m_Objective, id, now);
						remove.Insert(id);
						continue;
					}
				}
				else if (travels && (!e.m_bOffered || e.m_bOfferAnswered))
				{
					RecalcOnVehicle(cmd, grp, id, e, from);
					if (now > e.m_fEtaDeadline_s && !e.m_bArrived && !Arrived(grp, e.m_vPos, ArriveRadius(e)))
						GoLate(ts, cmd, grp, id, e);
				}
			}

			if (e.m_eType == DCO_EPlayerTaskType.MOVE && e.m_Objective && cmd.GetPlayerEarlyRelease()
				&& vector.DistanceXZ(from, e.m_Objective.GetOwner().GetOrigin()) <= e.m_Objective.GetRadius() && !early.Contains(e.m_Objective))
				early.Insert(e.m_Objective);

			if (e.m_eType == DCO_EPlayerTaskType.MOVE && now - e.m_fLastDesc_s >= 15.0)
			{
				e.m_fLastDesc_s = now;
				e.m_Task.SetTaskDescription(BuildDescription(cmd, e));
			}
		}

		foreach (int id : remove)
		{
			DCO_PlayerTaskEntry e = m_mTasks.Get(id);
			if (e && !e.m_bDone)
				Finish(ts, e, cmd);
			m_mTasks.Remove(id);
		}

		foreach (CMD_AICommanderObjectiveComponent obj : early)
			cmd.ReleaseAssaultEarly(obj);
	}

	void OnAssaultReleased(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		SCR_TaskSystem ts = SCR_TaskSystem.GetInstance();
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (!ts || !groups)
			return;

		array<int> ids = {};
		foreach (int id, DCO_PlayerTaskEntry e : m_mTasks)
		{
			if (e.m_Objective == obj && e.m_eType == DCO_EPlayerTaskType.MOVE)
				ids.Insert(id);
		}

		foreach (int id : ids)
		{
			SCR_AIGroup grp = groups.FindGroup(id);
			if (!grp)
				continue;

			DCO_Radio.Group(id, "COMMANDER", "assault_go", DCO_ERadioKind.GO, DCO_Radio.P("obj", obj.GetDisplayName()));
			UpdateGroup(ts, cmd, grp, id);
		}

		foreach (int rid, DCO_PlayerTaskEntry re : m_mTasks)
		{
			if (re.m_Objective != obj || re.m_eType != DCO_EPlayerTaskType.ROLE || re.m_bDone)
				continue;
			if (re.m_iRole == DCO_CommanderTactics.ROLE_FIX)
				DCO_Radio.Group(rid, "COMMANDER", "assault_go_sbf", DCO_ERadioKind.GO, DCO_Radio.P("obj", obj.GetDisplayName()));
			else if (re.m_iRole == DCO_CommanderTactics.ROLE_BLOCK)
				DCO_Radio.Group(rid, "COMMANDER", "assault_go_block", DCO_ERadioKind.GO, DCO_Radio.P("obj", obj.GetDisplayName()));
		}
	}

	bool HasActiveTask(CMD_AICommanderObjectiveComponent obj)
	{
		foreach (int id, DCO_PlayerTaskEntry e : m_mTasks)
		{
			if (e.m_Objective == obj && !e.m_bDone && e.m_bAccepted)
				return true;
		}
		return false;
	}

	bool IsBlockingSync(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (!groups)
			return false;

		float now = DCO_Util.NowS();
		foreach (int id, DCO_PlayerTaskEntry e : m_mTasks)
		{
			if (!e.m_bCounted || e.m_bLate || e.m_bDone || e.m_Objective != obj || e.m_eType != DCO_EPlayerTaskType.MOVE)
				continue;

			if (!e.m_bAccepted)
			{
				if (now - e.m_fCreated_s <= cmd.GetPlayerAcceptTime())
					return true;
				continue;
			}

			SCR_AIGroup grp = groups.FindGroup(id);
			if (grp && !Arrived(grp, e.m_vPos, ArriveRadius(e)))
				return true;
		}
		return false;
	}

	int GetCountedManpower()
	{
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (!groups)
			return 0;

		int total;
		foreach (int id, DCO_PlayerTaskEntry e : m_mTasks)
		{
			if (!e.m_bCounted || !e.m_bAccepted)
				continue;
			SCR_AIGroup grp = groups.FindGroup(id);
			if (grp)
				total += grp.GetPlayerCount() + grp.GetAgentsCount();
		}
		return total;
	}

	void GetGroupsOnObjective(CMD_AICommanderObjectiveComponent obj, notnull array<int> outIds)
	{
		foreach (int id, DCO_PlayerTaskEntry e : m_mTasks)
		{
			if (e.m_Objective == obj && !e.m_bDone)
				outIds.Insert(id);
		}
	}

	CMD_AICommanderObjectiveComponent GetGroupObjective(int groupID)
	{
		DCO_PlayerTaskEntry e = m_mTasks.Get(groupID);
		if (!e || e.m_bDone)
			return null;
		return e.m_Objective;
	}

	bool IsRoleWaited(int groupID)
	{
		DCO_PlayerTaskEntry e = m_mTasks.Get(groupID);
		return e && e.m_eType == DCO_EPlayerTaskType.ROLE && e.m_bCounted && !e.m_bLate && !e.m_bDone;
	}

	bool IsRoleArrived(int groupID)
	{
		DCO_PlayerTaskEntry e = m_mTasks.Get(groupID);
		return e && e.m_eType == DCO_EPlayerTaskType.ROLE && e.m_bArrived;
	}

	vector GroupLeaderPos(int groupID)
	{
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (!groups)
			return vector.Zero;
		SCR_AIGroup grp = groups.FindGroup(groupID);
		if (!grp)
			return vector.Zero;
		return LeaderPos(grp);
	}

	void Clear()
	{
		SCR_TaskSystem ts = SCR_TaskSystem.GetInstance();
		if (ts)
		{
			foreach (int id, DCO_PlayerTaskEntry e : m_mTasks)
				Finish(ts, e, null);
		}
		m_mTasks.Clear();
		m_mOffer.Clear();
		m_mOfferQueue.Clear();
	}

	int BestRoleCandidate(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, int bit, vector pos, int role, bool small, bool fireSupport, out float outScore)
	{
		outScore = -1000;
		if (!cmd.IsPlayerTaskingCounted())
			return -1;

		array<SCR_AIGroup> eligible = {};
		Eligible(cmd, eligible);

		float now = DCO_Util.NowS();
		int counted = CountedOn(obj, -1);
		int best = -1;
		foreach (SCR_AIGroup grp : eligible)
		{
			int id = grp.GetGroupID();
			if ((grp.DCO_GetPlayerRoles() & bit) == 0)
				continue;

			DCO_PlayerReliability rel = m_mReliability.Get(id);
			if (rel && (now < rel.m_fBonusUntil_s || now < rel.m_fCooldownUntil_s))
				continue;

			DCO_PlayerTaskEntry e = m_mTasks.Get(id);
			if (e && !e.m_bDone && e.m_eType == DCO_EPlayerTaskType.ROLE)
				continue;
			if (e && !e.m_bDone && e.m_bAccepted && e.m_Objective != obj)
				continue;

			DCO_PlayerRoleExclude ex = ActiveExclude(id);
			if (ex && ex.m_Obj == obj && (ex.m_iBits & bit) != 0)
				continue;

			int already = counted;
			if (e && e.m_bCounted && !e.m_bDone && e.m_Objective == obj)
				already--;
			if (already >= cmd.GetPlayerMaxPerObjective())
				continue;

			int units;
			bool mg, vehicle;
			Composition(grp, units, mg, vehicle);
			float v = DCO_CommanderTactics.RoleScore(units, mg, vehicle, vector.DistanceXZ(LeaderPos(grp), pos), role, small, fireSupport);
			if (DCO_CommanderTactics.IsDecisive(role))
				v += 1.5;
			if (v > outScore)
			{
				outScore = v;
				best = id;
			}
		}
		return best;
	}

	bool AssignRole(AICommander_BaseComponent cmd, int id, CMD_AICommanderObjectiveComponent obj, int tactic, int role, vector goal, vector retreatDir)
	{
		SCR_TaskSystem ts = SCR_TaskSystem.GetInstance();
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (!ts || !groups || !obj)
			return false;

		SCR_AIGroup grp = groups.FindGroup(id);
		if (!grp)
			return false;

		DCO_PlayerTaskEntry old = m_mTasks.Get(id);
		if (old)
		{
			if (!old.m_bDone)
				Finish(ts, old, cmd);
			else
				ReleaseSlot(cmd, old);
			m_mTasks.Remove(id);
		}

		vector from = LeaderPos(grp);
		DCO_PlayerTaskEntry e = new DCO_PlayerTaskEntry();
		e.m_Objective = obj;
		e.m_eType = DCO_EPlayerTaskType.ROLE;
		e.m_iRole = role;
		e.m_eTactic = tactic;
		e.m_vRetreat = retreatDir;
		e.m_vPos = goal;
		e.m_bCounted = true;
		e.m_fCreated_s = DCO_Util.NowS();
		e.m_fLastDesc_s = e.m_fCreated_s;
		e.m_fStartDist = vector.DistanceXZ(from, goal);
		e.m_bPending = true;

		m_mTasks.Insert(id, e);
		DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("player_role_assigned grp=%1 role=%2 tactic=%3 obj=%4 counted=%5 dist=%6",
			id, DCO_CommanderTactics.RoleName(role), DCO_CommanderTactics.Name(tactic), obj.GetLogName(), e.m_bCounted, Math.Round(e.m_fStartDist)));
		QueueOffer(cmd, grp, id, e, DCO_EOfferType.ROLE);
		return true;
	}

	void RolePhase(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, map<int, int> roles, string key, vector pos)
	{
		SCR_TaskSystem ts = SCR_TaskSystem.GetInstance();
		if (!ts || !roles)
			return;

		foreach (int id, int r : roles)
		{
			DCO_PlayerTaskEntry e = m_mTasks.Get(id);
			if (!e || e.m_eType != DCO_EPlayerTaskType.ROLE || e.m_Objective != obj || !e.m_Task || e.m_bDone)
				continue;

			e.m_vPos = pos;
			e.m_bArrived = false;
			ts.MoveTask(e.m_Task, pos);
			e.m_Task.SetTaskDescription(DCO_Radio.Resolve(key, 0, null) + "\n" + BuildDescription(cmd, e));
			DCO_Radio.Group(id, "COMMANDER", key, DCO_ERadioKind.WARNING);
		}
	}

	void OnRoleDisabled(AICommander_BaseComponent cmd, SCR_AIGroup grp, int bit)
	{
		int id = grp.GetGroupID();
		DCO_PlayerTaskEntry e = m_mTasks.Get(id);
		if (e && !e.m_bDone && EntryBit(e) == bit)
			DropForOther(cmd, grp, id, e, "toggle");
	}

	void RequestOther(AICommander_BaseComponent cmd, SCR_AIGroup grp)
	{
		int id = grp.GetGroupID();
		float now = DCO_Util.NowS();
		float cd;
		if (m_mOtherCd.Find(id, cd) && now < cd)
		{
			DCO_Radio.Group(id, "COMMANDER", "other_wait", DCO_ERadioKind.WARNING, DCO_Radio.P("sec", DCO_Radio.N(cd - now)));
			return;
		}

		DCO_PlayerTaskEntry e = m_mTasks.Get(id);
		if (!e || e.m_bDone)
		{
			DCO_Radio.Group(id, "COMMANDER", "other_none");
			return;
		}

		m_mOtherCd.Set(id, now + 60.0);
		DropForOther(cmd, grp, id, e, "request");
	}

	void AcceptTransport(AICommander_BaseComponent cmd, SCR_AIGroup grp, int pid)
	{
		int id = grp.GetGroupID();
		DCO_PlayerTaskEntry e = m_mTasks.Get(id);
		if (!e || e.m_bDone || !e.m_bOffered || e.m_bOfferAnswered)
		{
			DCO_Radio.Group(id, "LOGISTICS", "offer_none");
			return;
		}

		e.m_bOfferAnswered = true;
		DCO_PlayerOffer tro = m_mOffer.Get(id);
		if (tro && tro.m_iId == e.m_iOfferId)
		{
			e.m_iOfferId = 0;
			CloseOffer(cmd, tro, DCO_EOfferResult.ACCEPTED);
		}
		else if (e.m_iOfferId > 0)
			CancelOffer(cmd, e);
		DCO_Logistics logi = cmd.GetLogistics();
		if (!logi || !logi.RequestRoleTransport(cmd, pid, grp, LeaderPos(grp), e.m_vPos))
		{
			DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("player_transport_none grp=%1 reason=request_failed", id));
			MakeBonus(cmd, id, e, "unavailable", "@bonus_unavailable");
			return;
		}

		e.m_fEtaDeadline_s = DCO_Util.NowS() + cmd.GetPlayerEtaMargin() + e.m_fOfferEta;
		DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("player_transport_accept grp=%1 eta=%2", id, Math.Round(e.m_fOfferEta)));
		DCO_Radio.Group(id, "LOGISTICS", "transport_accepted", DCO_ERadioKind.INFO, DCO_Radio.P("eta", DCO_Radio.N(Math.Max(1, e.m_fOfferEta / 60))));
	}

	protected void DropForOther(AICommander_BaseComponent cmd, SCR_AIGroup grp, int id, DCO_PlayerTaskEntry e, string why)
	{
		int bit = EntryBit(e);
		string objName = "-";
		if (e.m_Objective)
			objName = e.m_Objective.GetLogName();
		DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("player_role_request_other grp=%1 role=%2 obj=%3 reason=%4", id, EntryRoleLabel(e), objName, why));

		DCO_PlayerRoleExclude ex = new DCO_PlayerRoleExclude();
		ex.m_Obj = e.m_Objective;
		ex.m_iBits = bit;
		ex.m_fUntil_s = DCO_Util.NowS() + 300.0;
		m_mExclude.Set(id, ex);

		string taskName = "the task";
		if (e.m_Task)
			taskName = e.m_Task.GetTaskName();

		SCR_TaskSystem ts = SCR_TaskSystem.GetInstance();
		Finish(ts, e, cmd);
		m_mTasks.Remove(id);
		if (e.m_eType == DCO_EPlayerTaskType.ROLE && e.m_Objective)
			cmd.GetTactics().RefillRole(cmd, e.m_Objective, id, DCO_Util.NowS());

		DCO_Radio.Group(id, "COMMANDER", "other_handed", DCO_ERadioKind.INFO, DCO_Radio.P("task", taskName));
		if (ts)
			UpdateGroup(ts, cmd, grp, id);
	}

	protected void UpdateGroup(SCR_TaskSystem ts, AICommander_BaseComponent cmd, SCR_AIGroup grp, int id)
	{
		vector from = LeaderPos(grp);
		DCO_PlayerTaskEntry e = m_mTasks.Get(id);

		if (e && e.m_eType == DCO_EPlayerTaskType.ROLE)
		{
			if (e.m_Objective && cmd.GetTactics().HasPlayerRole(e.m_Objective, id, e.m_iRole))
			{
				if (!e.m_bDone && e.m_Task && IsAccomplished(e, cmd, grp))
				{
					Finish(ts, e, cmd, true);
					RecordSuccess(id);
				}
				return;
			}

			if (!e.m_bDone)
			{
				bool ok = e.m_bArrived || IsAccomplished(e, cmd, grp);
				Finish(ts, e, cmd, ok);
				if (ok)
					RecordSuccess(id);
			}
			m_mTasks.Remove(id);
			e = null;
		}

		CMD_AICommanderObjectiveComponent obj;
		DCO_EPlayerTaskType type;
		vector pos;
		bool flank;
		bool has = FindTarget(cmd, grp, id, from, obj, type, pos, flank);

		if (e && has && e.m_Objective == obj && e.m_eType == type)
		{
			if (!e.m_bDone && e.m_Task && IsAccomplished(e, cmd, grp))
			{
				Finish(ts, e, cmd, true, e.m_eType == DCO_EPlayerTaskType.MOVE);
				RecordSuccess(id);
			}
			if (e.m_Task || e.m_bDone || e.m_bPending)
				return;
		}

		bool keepCounted, keepAccepted;
		if (e)
		{
			bool advanced = has && e.m_eType == DCO_EPlayerTaskType.MOVE && type == DCO_EPlayerTaskType.ATTACK && e.m_Objective == obj;
			keepCounted = advanced && e.m_bCounted && !e.m_bLate;
			keepAccepted = advanced && e.m_bAccepted;
			bool slot = keepCounted && e.m_bSlot;
			if (slot)
				e.m_bSlot = false;
			else
				ReleaseSlot(cmd, e);

			if (!e.m_bDone)
				Finish(ts, e, cmd, advanced || IsAccomplished(e, cmd, grp));
			m_mTasks.Remove(id);

			if (has)
			{
				e = NewEntry(cmd, id, obj, type, pos, from, keepCounted, flank);
				if (e && keepAccepted)
				{
					e.m_bAccepted = true;
					e.m_bSlot = slot;
					ActivateWithoutOffer(ts, cmd, id, e);
				}
				return;
			}
		}

		if (!has)
			return;

		NewEntry(cmd, id, obj, type, pos, from, CanCount(cmd, id, obj, type), flank);
	}

	protected bool FindTarget(AICommander_BaseComponent cmd, SCR_AIGroup grp, int id, vector from, out CMD_AICommanderObjectiveComponent obj, out DCO_EPlayerTaskType type, out vector pos, out bool flank)
	{
		flank = false;
		array<CMD_AICommanderObjectiveComponent> skip = {};
		if ((grp.DCO_GetPlayerRoles() & 1) == 0)
		{
			foreach (CMD_AICommanderObjectiveComponent o : cmd.GetObjectiveList())
				skip.Insert(o);
		}

		DCO_PlayerRoleExclude ex = ActiveExclude(id);
		if (ex && (ex.m_iBits & 1) != 0)
			skip.Insert(ex.m_Obj);

		array<CMD_AICommanderObjectiveComponent> spread = {};
		spread.InsertAll(skip);
		foreach (CMD_AICommanderObjectiveComponent o2 : cmd.GetObjectiveList())
		{
			if (o2 && CountedOn(o2, id) >= cmd.GetPlayerMaxPerObjective())
				spread.Insert(o2);
		}

		bool has = cmd.GetPlayerTaskTarget(from, obj, type, pos, spread);
		if (!has)
			has = cmd.GetPlayerTaskTarget(from, obj, type, pos, skip);
		if (has && type == DCO_EPlayerTaskType.DEFEND && ex && ex.m_Obj == obj)
			return false;

		if (has && type == DCO_EPlayerTaskType.MOVE && obj)
		{
			DCO_TacticPlan plan = cmd.GetTactics().GetPlan(obj);
			if (plan && (plan.m_eTactic == DCO_ETactic.FLANKING || plan.m_eTactic == DCO_ETactic.PINCER || plan.m_eTactic == DCO_ETactic.HAMMER_AND_ANVIL))
			{
				vector objPos = obj.GetOwner().GetOrigin();
				int side = 1;
				if (id % 2 != 0)
					side = -1;
				pos = cmd.FlankPoint(pos, objPos, vector.DistanceXZ(pos, objPos), 70.0, side);
				flank = true;
			}
		}
		return has;
	}

	protected DCO_PlayerTaskEntry NewEntry(AICommander_BaseComponent cmd, int id, CMD_AICommanderObjectiveComponent obj, DCO_EPlayerTaskType type, vector pos, vector from, bool counted, bool flank)
	{
		SCR_TaskSystem ts = SCR_TaskSystem.GetInstance();
		DCO_PlayerTaskEntry e = new DCO_PlayerTaskEntry();
		e.m_Objective = obj;
		e.m_eType = type;
		e.m_vPos = pos;
		e.m_bCounted = counted;
		e.m_bFlank = flank;
		e.m_fCreated_s = DCO_Util.NowS();
		e.m_fLastDesc_s = e.m_fCreated_s;
		e.m_fStartDist = vector.DistanceXZ(from, pos);
		e.m_bPending = true;

		m_mTasks.Insert(id, e);
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (groups)
			QueueOffer(cmd, groups.FindGroup(id), id, e, DCO_EOfferType.TASK);
		return e;
	}

	protected bool CanCount(AICommander_BaseComponent cmd, int id, CMD_AICommanderObjectiveComponent obj, DCO_EPlayerTaskType type)
	{
		if (!cmd.IsPlayerTaskingCounted() || type == DCO_EPlayerTaskType.DEFEND)
			return false;

		DCO_PlayerReliability rel = m_mReliability.Get(id);
		if (rel && DCO_Util.NowS() < rel.m_fBonusUntil_s)
			return false;

		return CountedOn(obj, id) < cmd.GetPlayerMaxPerObjective();
	}

	protected int CountedOn(CMD_AICommanderObjectiveComponent obj, int excludeId)
	{
		int n;
		foreach (int other, DCO_PlayerTaskEntry e : m_mTasks)
		{
			if (other != excludeId && e.m_bCounted && e.m_Objective == obj && !e.m_bDone)
				n++;
		}
		return n;
	}

	protected void Accept(AICommander_BaseComponent cmd, SCR_AIGroup grp, int id, DCO_PlayerTaskEntry e, vector from)
	{
		e.m_bAccepted = true;
		e.m_bInVehicle = DCO_VehicleCombat.GetVehicle(grp.GetLeaderEntity()) != null;

		float calc = TravelTime(cmd, vector.DistanceXZ(from, e.m_vPos), e.m_bInVehicle);
		float buffer = cmd.GetPlayerEtaMargin();
		bool capped = calc > cmd.GetPlayerMaxEtaMin() * 60;
		e.m_fEtaDeadline_s = DCO_Util.NowS() + buffer + calc;
		DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("player_eta grp=%1 calc=%2 buffer=%3 capped=%4 vehicle=%5 reason=accept", id, Math.Round(calc), Math.Round(buffer), capped, e.m_bInVehicle));

		if (e.m_Objective && e.m_eType != DCO_EPlayerTaskType.DEFEND && !e.m_bSlot)
		{
			e.m_Objective.SetObjectiveGroup(cmd.GetCommanderFactionKey(), 1);
			e.m_bSlot = true;
		}

		DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("group %1 accepted %2 (counted, ETA %3 s)", id, e.m_Task.GetTaskName(), Math.Round(e.m_fEtaDeadline_s - DCO_Util.NowS())));

		if (capped)
			OfferTransport(cmd, grp, id, e, from);
	}

	protected float TravelTime(AICommander_BaseComponent cmd, float dist, bool vehicle)
	{
		float speed = 1.8;
		if (vehicle)
			speed = 8.0;
		return dist / speed * cmd.GetPlayerEtaFactor();
	}

	protected void RecalcOnVehicle(AICommander_BaseComponent cmd, SCR_AIGroup grp, int id, DCO_PlayerTaskEntry e, vector from)
	{
		bool inVeh = DCO_VehicleCombat.GetVehicle(grp.GetLeaderEntity()) != null;
		if (inVeh == e.m_bInVehicle)
			return;

		e.m_bInVehicle = inVeh;
		float calc = TravelTime(cmd, vector.DistanceXZ(from, e.m_vPos), inVeh);
		float buffer = cmd.GetPlayerEtaMargin();
		e.m_fEtaDeadline_s = DCO_Util.NowS() + buffer + calc;
		string why = "dismount";
		if (inVeh)
			why = "mount";
		DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("player_eta grp=%1 calc=%2 buffer=%3 capped=0 vehicle=%4 reason=%5", id, Math.Round(calc), Math.Round(buffer), inVeh, why));
	}

	protected void OfferTransport(AICommander_BaseComponent cmd, SCR_AIGroup grp, int id, DCO_PlayerTaskEntry e, vector from)
	{
		float pickup;
		float eta = -1;
		if (cmd.GetLogistics())
			eta = cmd.GetLogistics().EstimatePlayerTransport(cmd, grp, from, e.m_vPos, pickup);

		if (eta < 0)
		{
			DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("player_transport_none grp=%1 reason=no_team", id));
			MakeBonus(cmd, id, e, "no_transport", "@bonus_no_transport");
			return;
		}

		e.m_bOffered = true;
		e.m_fOfferAt_s = DCO_Util.NowS();
		e.m_fOfferEta = eta;
		e.m_fEtaDeadline_s = DCO_Util.NowS() + cmd.GetPlayerAcceptTime() + cmd.GetPlayerEtaMargin() + eta;
		DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("player_transport_offer grp=%1 pickup=%2 eta=%3", id, Math.Round(pickup), Math.Round(eta)));
		DCO_Radio.Group(id, "COMMANDER", "offer_transport", DCO_ERadioKind.WARNING, DCO_Radio.P("eta", DCO_Radio.N(Math.Max(1, pickup / 60))));
		QueueOffer(cmd, grp, id, e, DCO_EOfferType.TRANSPORT, pickup);
	}

	protected void MakeBonus(AICommander_BaseComponent cmd, int id, DCO_PlayerTaskEntry e, string why, string reasonKey)
	{
		e.m_bLate = true;
		ReleaseSlot(cmd, e);
		if (e.m_Task)
			e.m_Task.SetTaskDescription(BuildDescription(cmd, e));
		DCO_Radio.Group(id, "COMMANDER", "task_bonus", DCO_ERadioKind.WARNING, DCO_Radio.P("reason", reasonKey));
		DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("player_role_bonus grp=%1 reason=\"%2\"", id, why));
	}

	protected void GoLate(SCR_TaskSystem ts, AICommander_BaseComponent cmd, SCR_AIGroup grp, int id, DCO_PlayerTaskEntry e)
	{
		RecordFail(cmd, id, false);

		if (e.m_eType == DCO_EPlayerTaskType.ROLE)
		{
			MakeBonus(cmd, id, e, "late", "@bonus_late");
			return;
		}

		e.m_bLate = true;
		ReleaseSlot(cmd, e);

		if (e.m_Objective)
		{
			string objName = e.m_Objective.GetDisplayName();
			e.m_eType = DCO_EPlayerTaskType.ATTACK;
			e.m_vPos = e.m_Objective.GetOwner().GetOrigin();
			e.m_Task.SetTaskName("Attack " + objName);
			e.m_Task.SetTaskDescription("The assault on " + objName + " is going ahead. Push in and support it.");
			e.m_Task.SetTaskIconSetName("Icon_Task_Seize");
			ts.MoveTask(e.m_Task, e.m_vPos);

			DCO_Radio.Group(id, "COMMANDER", "task_late", DCO_ERadioKind.WARNING, DCO_Radio.P("callsign", DCO_PlayerComms.GetCallsign(grp), "obj", objName));
		}
		DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("group %1 past ETA -> no longer awaited, becomes ATTACK bonus", id));
	}

	protected void UpdateRoleArrival(SCR_AIGroup grp, int id, DCO_PlayerTaskEntry e)
	{
		if (e.m_bArrived || !Arrived(grp, e.m_vPos, ArriveRadius(e)))
			return;

		e.m_bArrived = true;
		string objName = e.m_Objective.GetDisplayName();
		string msg;
		switch (e.m_iRole)
		{
			case DCO_CommanderTactics.ROLE_FIX:
				msg = "arrived_fix";
				break;
			case DCO_CommanderTactics.ROLE_BLOCK:
				msg = "arrived_block";
				break;
			case 1:
				msg = "arrived_strike";
				break;
		}
		if (!msg.IsEmpty())
			DCO_Radio.Group(id, "COMMANDER", msg, DCO_ERadioKind.GO, DCO_Radio.P("obj", objName));
		DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("player_role_arrived grp=%1 role=%2 obj=%3", id, DCO_CommanderTactics.RoleName(e.m_iRole), e.m_Objective.GetLogName()));
	}

	protected SCR_Task CreateTask(SCR_TaskSystem ts, AICommander_BaseComponent cmd, int groupID, DCO_PlayerTaskEntry e)
	{
		string icon;
		string name = TaskName(e, icon);

		m_iSeq++;
		string taskID = string.Format("DCO_%1_%2_%3", cmd.GetCommanderUID(), groupID, m_iSeq);
		SCR_Task task = ts.CreateTask("{1D0F815858EE24AD}Prefabs/Tasks/BaseTask.et", taskID, name, BuildDescription(cmd, e), e.m_vPos);
		if (!task)
			return null;

		task.SetTaskIconPath("{10C0A9A305E8B3A4}UI/Imagesets/Tasks/Task_Icons.imageset");
		task.SetTaskIconSetName(icon);
		ts.SetTaskOwnership(task, SCR_ETaskOwnership.GROUP);
		ts.SetTaskVisibility(task, SCR_ETaskVisibility.GROUP);
		ts.SetTaskUIVisibility(task, SCR_ETaskUIVisibility.ALL);
		ts.AddTaskFaction(task, cmd.GetCommanderFactionKey());
		ts.AddTaskGroup(task, groupID);
		ts.AssignTask(task, SCR_TaskSystem.TaskExecutorFromGroup(groupID), true);
		DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("%1 group %2: %3 (counted %4)", taskID, groupID, name, e.m_bCounted));
		return task;
	}

	protected string TaskName(DCO_PlayerTaskEntry e, out string icon)
	{
		string objName = e.m_Objective.GetDisplayName();
		switch (e.m_eType)
		{
			case DCO_EPlayerTaskType.MOVE:
				icon = "Icon_Task_Move";
				if (e.m_bFlank)
					return "Maneuver: flank " + objName;
				return "Stage for " + objName;
			case DCO_EPlayerTaskType.ATTACK:
				icon = "Icon_Task_Seize";
				return "Attack " + objName;
			case DCO_EPlayerTaskType.DEFEND:
				icon = "Icon_Task_Defend";
				return "Defend " + objName;
		}

		string dir = DCO_PlayerComms.Bearing(e.m_Objective.GetOwner().GetOrigin(), e.m_vPos);
		switch (e.m_iRole)
		{
			case DCO_CommanderTactics.ROLE_FIX:
				icon = "Icon_Task_Defend";
				return "SBF: suppress " + objName;
			case DCO_CommanderTactics.ROLE_BLOCK:
				icon = "Icon_Task_Defend";
				if (e.m_eTactic == DCO_ETactic.HAMMER_AND_ANVIL)
					return "Anvil: block the " + dir + " of " + objName;
				return "Block: cut off the " + dir + " of " + objName;
			case 1:
				icon = "Icon_Task_Move";
				return "Infiltrate " + objName;
		}

		icon = "Icon_Task_Seize";
		if (e.m_eTactic == DCO_ETactic.RAID)
			return "Raid " + objName;
		if (e.m_eTactic == DCO_ETactic.RECON_IN_FORCE)
			return "Probe " + objName;
		return "Assault " + objName;
	}

	protected string BuildDescription(AICommander_BaseComponent cmd, DCO_PlayerTaskEntry e)
	{
		string objName = e.m_Objective.GetDisplayName();
		string desc;
		switch (e.m_eType)
		{
			case DCO_EPlayerTaskType.MOVE:
				if (e.m_bFlank)
					desc = "Move to the flank position " + DCO_PlayerComms.Bearing(e.m_Objective.GetOwner().GetOrigin(), e.m_vPos) + " of " + objName + " and wait for the assault. Hit them from the side when it goes in.";
				else
					desc = "Move to the staging area and hold for the coordinated assault on " + objName + ".";
				break;
			case DCO_EPlayerTaskType.ATTACK:
				desc = "Assault and capture " + objName + " with the other friendly groups.";
				break;
			case DCO_EPlayerTaskType.DEFEND:
				return objName + " is under attack. Hold it.";
			default:
				desc = RoleDescription(e, objName);
				break;
		}

		string groupsLine = cmd.DescribeOperationGroups(e.m_Objective);
		if (!groupsLine.IsEmpty())
			desc += "\nWith: " + groupsLine + ".";

		if (e.m_eType == DCO_EPlayerTaskType.MOVE)
		{
			float countdown = cmd.GetAssaultCountdown(e.m_Objective);
			if (countdown > 0)
				desc += string.Format("\nAssault starts in about %1 s.", Math.Round(countdown));
			else if (countdown < 0)
				desc += "\nAssault starts when the groups are in position.";
		}

		if (!e.m_bCounted || e.m_bLate)
			desc += "\n(Optional: the operation does not wait for you.)";
		return desc;
	}

	protected string RoleDescription(DCO_PlayerTaskEntry e, string objName)
	{
		switch (e.m_iRole)
		{
			case DCO_CommanderTactics.ROLE_FIX:
				if (e.m_eTactic == DCO_ETactic.SIEGE || e.m_eTactic == DCO_ETactic.ATTACK_BY_FIRE)
					return "Support by fire: get into position and keep " + objName + " pinned with fire. Do not assault, the commander wears them down first.";
				return "Support by fire: suppress " + objName + " from here until the assault goes in, then shift fire.";
			case DCO_CommanderTactics.ROLE_BLOCK:
				if (e.m_eTactic == DCO_ETactic.HAMMER_AND_ANVIL)
					return "Anvil: block the escape route " + DCO_PlayerComms.Bearing(e.m_Objective.GetOwner().GetOrigin(), e.m_vPos) + " of " + objName + ". Stay hidden and hold fire until the enemy is in the kill zone. The assault waits until you are in position.";
				return "Block: cut the route " + DCO_PlayerComms.Bearing(e.m_Objective.GetOwner().GetOrigin(), e.m_vPos) + " of " + objName + ". Stop reinforcements coming in and the enemy getting out.";
			case 1:
				return "Infiltration: move covered and quiet to the key point inside " + objName + ". Avoid contact until you are in position, then strike.";
		}

		if (e.m_eTactic == DCO_ETactic.RAID)
			return "Raid: hit " + objName + " hard and fast, then pull back when ordered.";
		if (e.m_eTactic == DCO_ETactic.RECON_IN_FORCE)
			return "Recon in force: push into " + objName + " and find the enemy positions. Pull back when ordered or if losses get heavy.";
		return "Assault and capture " + objName + ".";
	}

	protected bool IsAccomplished(DCO_PlayerTaskEntry e, AICommander_BaseComponent cmd, SCR_AIGroup grp)
	{
		if (!cmd || !e.m_Objective)
			return false;

		switch (e.m_eType)
		{
			case DCO_EPlayerTaskType.MOVE:
				return grp && (Arrived(grp, e.m_vPos, ArriveRadius(e)) || Arrived(grp, e.m_Objective.GetOwner().GetOrigin(), ArriveRadius(e)));
			case DCO_EPlayerTaskType.ATTACK:
			case DCO_EPlayerTaskType.ROLE:
				return e.m_Objective.IsCapturedBy(cmd.GetCommanderFactionKey());
			case DCO_EPlayerTaskType.DEFEND:
				return e.m_Objective.IsCapturedBy(cmd.GetCommanderFactionKey()) && !cmd.IsObjectiveContested(e.m_Objective);
		}
		return false;
	}

	protected float ArriveRadius(DCO_PlayerTaskEntry e)
	{
		if (!e.m_Objective)
			return 40.0;
		return Math.Max(e.m_Objective.GetRadius(), 40.0);
	}

	protected int EntryBit(DCO_PlayerTaskEntry e)
	{
		if (e.m_eType == DCO_EPlayerTaskType.ROLE)
			return DCO_CommanderTactics.RoleBit(e.m_eTactic, e.m_iRole);
		if (e.m_eType == DCO_EPlayerTaskType.DEFEND)
			return 0;
		return 1;
	}

	protected string EntryRoleLabel(DCO_PlayerTaskEntry e)
	{
		if (e.m_eType == DCO_EPlayerTaskType.ROLE)
			return DCO_CommanderTactics.RoleName(e.m_iRole);
		return typename.EnumToString(DCO_EPlayerTaskType, e.m_eType);
	}

	protected DCO_PlayerRoleExclude ActiveExclude(int id)
	{
		DCO_PlayerRoleExclude ex = m_mExclude.Get(id);
		if (!ex)
			return null;
		if (DCO_Util.NowS() > ex.m_fUntil_s || !ex.m_Obj)
		{
			m_mExclude.Remove(id);
			return null;
		}
		return ex;
	}

	protected void Eligible(AICommander_BaseComponent cmd, notnull array<SCR_AIGroup> outGroups)
	{
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		FactionManager fm = GetGame().GetFactionManager();
		if (!groups || !fm)
			return;

		Faction faction = fm.GetFactionByKey(cmd.GetCommanderFactionKey());
		if (!faction)
			return;

		array<SCR_AIGroup> playable = groups.GetPlayableGroupsByFaction(faction);
		if (!playable)
			return;

		foreach (SCR_AIGroup grp : playable)
		{
			if (grp && grp.GetLeaderID() > 0 && !grp.DCO_IsDecliningOrders() && grp.DCO_GetOrdersCommander() == cmd)
				outGroups.Insert(grp);
		}
	}

	protected static void Members(SCR_AIGroup grp, notnull array<IEntity> outList)
	{
		PlayerManager pm = GetGame().GetPlayerManager();
		array<int> pids = grp.GetPlayerIDs();
		if (pids)
		{
			foreach (int pid : pids)
			{
				IEntity ent = pm.GetPlayerControlledEntity(pid);
				if (DCO_Util.IsNotDead(ent))
					outList.Insert(ent);
			}
		}

		array<AIAgent> agents = {};
		grp.GetAgents(agents);
		if (grp.GetSlave())
			grp.GetSlave().GetAgents(agents);
		foreach (AIAgent a : agents)
		{
			IEntity ent = a.GetControlledEntity();
			if (DCO_Util.IsNotDead(ent) && !outList.Contains(ent))
				outList.Insert(ent);
		}
	}

	protected static bool Arrived(SCR_AIGroup grp, vector pos, float radius)
	{
		if (vector.DistanceXZ(LeaderPos(grp), pos) > radius)
			return false;

		array<IEntity> members = {};
		Members(grp, members);
		if (members.IsEmpty())
			return true;

		int inside;
		foreach (IEntity m : members)
		{
			if (vector.DistanceXZ(m.GetOrigin(), pos) <= radius)
				inside++;
		}
		return inside * 2 > members.Count();
	}

	protected static void Composition(SCR_AIGroup grp, out int units, out bool mg, out bool vehicle)
	{
		array<IEntity> members = {};
		Members(grp, members);
		units = members.Count();
		mg = false;
		vehicle = false;

		array<IEntity> weapons = {};
		foreach (IEntity m : members)
		{
			if (DCO_VehicleCombat.GetVehicle(m))
				vehicle = true;
			if (mg)
				continue;

			BaseWeaponManagerComponent wm = BaseWeaponManagerComponent.Cast(m.FindComponent(BaseWeaponManagerComponent));
			if (!wm)
				continue;

			weapons.Clear();
			wm.GetWeaponsList(weapons);
			foreach (IEntity w : weapons)
			{
				WeaponComponent wc = WeaponComponent.Cast(w.FindComponent(WeaponComponent));
				if (wc && wc.GetWeaponType() == EWeaponType.WT_MACHINEGUN)
				{
					mg = true;
					break;
				}
			}
		}
	}

	protected void QueueOffer(AICommander_BaseComponent cmd, SCR_AIGroup grp, int id, DCO_PlayerTaskEntry e, int type, float pickup = 0)
	{
		if (!grp || !e)
			return;
		DCO_PlayerOffer o = new DCO_PlayerOffer();
		m_iOfferSeq++;
		o.m_iId = m_iOfferSeq;
		o.m_iGroup = id;
		o.m_iType = type;
		o.m_fQueued_s = DCO_Util.NowS();
		o.m_sCallsign = DCO_PlayerComms.GetCallsign(grp);
		o.m_vPos = e.m_vPos;
		o.m_sObj = "-";
		string objName = "";
		if (e.m_Objective)
		{
			objName = e.m_Objective.GetDisplayName();
			o.m_sObj = objName;
		}
		if (e.m_eType == DCO_EPlayerTaskType.ROLE)
		{
			o.m_sTactic = DCO_CommanderTactics.Name(e.m_eTactic);
			o.m_sRole = DCO_CommanderTactics.RoleName(e.m_iRole);
		}
		else
			o.m_sRole = typename.EnumToString(DCO_EPlayerTaskType, e.m_eType);

		vector from = LeaderPos(grp);
		float dist = vector.DistanceXZ(from, e.m_vPos);
		string icon;
		if (type == DCO_EOfferType.TRANSPORT)
		{
			o.m_sTitle = TaskName(e, icon);
			o.m_sSub = string.Format("%1  ·  pickup in about %2 min", objName, Math.Max(1, Math.Round(pickup / 60)));
			o.m_sDesc = "Distance too far to walk in time. Accept and a vehicle picks your group up here.";
		}
		else
		{
			o.m_sTitle = TaskName(e, icon);
			if (!o.m_sTactic.IsEmpty())
				o.m_sTitle = o.m_sTitle + " (" + o.m_sTactic + ")";
			bool inVeh = DCO_VehicleCombat.GetVehicle(grp.GetLeaderEntity()) != null;
			float eta = TravelTime(cmd, dist, inVeh) + cmd.GetPlayerEtaMargin();
			o.m_sSub = string.Format("%1  ·  %2 m %3  ·  ETA %4 min", objName, Math.Round(dist), DCO_PlayerComms.Bearing(from, e.m_vPos), Math.Max(1, Math.Round(eta / 60)));
			string desc = BuildDescription(cmd, e);
			int nl = desc.IndexOf("\n");
			if (nl > 0)
				desc = desc.Substring(0, nl);
			o.m_sDesc = desc;
		}
		e.m_iOfferId = o.m_iId;

		array<ref DCO_PlayerOffer> q = m_mOfferQueue.Get(id);
		if (!q)
		{
			q = {};
			m_mOfferQueue.Insert(id, q);
		}
		int at = q.Count();
		foreach (int i, DCO_PlayerOffer other : q)
		{
			if (type < other.m_iType)
			{
				at = i;
				break;
			}
		}
		q.InsertAt(o, at);
	}

	protected void ProcessOffers(AICommander_BaseComponent cmd)
	{
		if (!cmd || !Replication.IsServer())
			return;
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (!groups)
			return;
		float now = DCO_Util.NowS();

		array<int> done = {};
		foreach (int gid, DCO_PlayerOffer o : m_mOffer)
		{
			SCR_AIGroup grp = groups.FindGroup(gid);
			if (!grp)
			{
				done.Insert(gid);
				continue;
			}
			if (now >= o.m_fExpire_s)
			{
				done.Insert(gid);
				continue;
			}
			SendOffer(grp, o, now);
		}
		foreach (int did : done)
		{
			DCO_PlayerOffer expired = m_mOffer.Get(did);
			if (expired)
				ResolveOffer(cmd, expired, DCO_EOfferResult.TIMEOUT, -1);
		}

		foreach (int qid, array<ref DCO_PlayerOffer> q : m_mOfferQueue)
		{
			if (!q || q.IsEmpty() || m_mOffer.Contains(qid))
				continue;
			float gap = 0;
			if (m_mOfferGap.Find(qid, gap) && now < gap)
				continue;
			SCR_AIGroup qg = groups.FindGroup(qid);
			if (!qg)
				continue;
			DCO_PlayerOffer next = q[0];
			q.RemoveOrdered(0);
			next.m_fSent_s = now;
			next.m_fExpire_s = now + cmd.GetPlayerAcceptTime();
			m_mOffer.Set(qid, next);
			SendOffer(qg, next, now);
			DCO_Radio.Group(qid, "COMMANDER", "offer_new", DCO_ERadioKind.INFO, DCO_Radio.P("callsign", next.m_sCallsign, "task", next.m_sTitle));
			DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("player_offer_sent grp=%1 type=%2 tactic=%3 role=%4 obj=%5 wait=%6",
				qid, typename.EnumToString(DCO_EOfferType, next.m_iType), NoneIfEmpty(next.m_sTactic), next.m_sRole, next.m_sObj, Math.Round(now - next.m_fQueued_s)));
		}
	}

	protected static string NoneIfEmpty(string s)
	{
		if (s.IsEmpty())
			return "-";
		return s;
	}

	protected void SendOffer(SCR_AIGroup grp, DCO_PlayerOffer o, float now)
	{
		int leader = grp.GetLeaderID();
		if (leader != o.m_iLeader)
		{
			o.m_iLeader = leader;
			o.m_aSentTo.Clear();
		}
		array<int> pids = grp.GetPlayerIDs();
		if (!pids)
			return;
		float total = o.m_fExpire_s - o.m_fSent_s;
		float remain = Math.Max(0, o.m_fExpire_s - now);
		foreach (int pid : pids)
		{
			if (o.m_aSentTo.Contains(pid))
				continue;
			SCR_PlayerControllerGroupComponent comp = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(pid);
			if (!comp)
				continue;
			comp.DCO_SendOffer(o.m_iId, o.m_iType, o.m_sCallsign, o.m_sTitle, o.m_sSub, o.m_sDesc, o.m_vPos[0], o.m_vPos[2], remain, total, pid == leader);
			o.m_aSentTo.Insert(pid);
		}
	}

	protected void CloseOffer(AICommander_BaseComponent cmd, DCO_PlayerOffer o, int result)
	{
		m_mOffer.Remove(o.m_iGroup);
		float gap = 30.0;
		if (result == DCO_EOfferResult.ACCEPTED)
			gap = 5.0;
		m_mOfferGap.Set(o.m_iGroup, DCO_Util.NowS() + gap);

		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		array<int> pids = {};
		pids.InsertAll(o.m_aSentTo);
		if (groups)
		{
			SCR_AIGroup grp = groups.FindGroup(o.m_iGroup);
			if (grp && grp.GetPlayerIDs())
			{
				foreach (int gp : grp.GetPlayerIDs())
				{
					if (!pids.Contains(gp))
						pids.Insert(gp);
				}
			}
		}
		foreach (int pid : pids)
		{
			SCR_PlayerControllerGroupComponent comp = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(pid);
			if (comp)
				comp.DCO_SendOfferClose(o.m_iId, result);
		}

		float answer = -1;
		if (o.m_fSent_s > 0)
			answer = DCO_Util.NowS() - o.m_fSent_s;
		string res = typename.EnumToString(DCO_EOfferResult, result);
		res.ToLower();
		DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("player_offer_%1 grp=%2 type=%3 tactic=%4 role=%5 obj=%6 answer_s=%7",
			res, o.m_iGroup, typename.EnumToString(DCO_EOfferType, o.m_iType), NoneIfEmpty(o.m_sTactic), o.m_sRole, o.m_sObj, Math.Round(answer)));
		DCO_PlayerComms.SendToGameMasters("DCO", string.Format("%1: offer %2 (%3) - %4", o.m_sCallsign, res, o.m_sTitle, o.m_sObj));
	}

	protected void ResolveOffer(AICommander_BaseComponent cmd, DCO_PlayerOffer o, int result, int pid)
	{
		CloseOffer(cmd, o, result);
		DCO_PlayerTaskEntry e = m_mTasks.Get(o.m_iGroup);
		if (!e || e.m_iOfferId != o.m_iId)
			return;
		e.m_iOfferId = 0;
		int id = o.m_iGroup;
		SCR_TaskSystem ts = SCR_TaskSystem.GetInstance();
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		SCR_AIGroup grp;
		if (groups)
			grp = groups.FindGroup(id);

		if (o.m_iType == DCO_EOfferType.TRANSPORT)
		{
			if (result == DCO_EOfferResult.ACCEPTED && grp)
			{
				AcceptTransport(cmd, grp, pid);
				return;
			}
			if (result == DCO_EOfferResult.CANCELLED)
				return;
			e.m_bOfferAnswered = true;
			DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("player_transport_none grp=%1 reason=%2", id, typename.EnumToString(DCO_EOfferResult, result)));
			MakeBonus(cmd, id, e, "no_transport", "@bonus_no_answer");
			return;
		}

		if (result == DCO_EOfferResult.CANCELLED)
			return;

		if (result == DCO_EOfferResult.ACCEPTED && grp && ts)
		{
			e.m_bPending = false;
			e.m_Task = CreateTask(ts, cmd, id, e);
			if (!e.m_Task)
			{
				if (e.m_eType == DCO_EPlayerTaskType.ROLE && e.m_Objective)
					cmd.GetTactics().RefillRole(cmd, e.m_Objective, id, DCO_Util.NowS());
				Finish(ts, e, cmd);
				m_mTasks.Remove(id);
				return;
			}
			e.m_fCreated_s = DCO_Util.NowS();
			if (e.m_bCounted && !e.m_bLate)
				Accept(cmd, grp, id, e, LeaderPos(grp));
			else
				e.m_bAccepted = true;
			DCO_Radio.Group(id, "COMMANDER", "offer_accepted", DCO_ERadioKind.INFO, DCO_Radio.P("callsign", o.m_sCallsign, "task", e.m_Task.GetTaskName()));
			return;
		}

		string key = "offer_declined";
		if (result == DCO_EOfferResult.TIMEOUT)
			key = "offer_timeout";
		DCO_PlayerRoleExclude ex = new DCO_PlayerRoleExclude();
		ex.m_Obj = e.m_Objective;
		ex.m_iBits = EntryBit(e);
		ex.m_fUntil_s = DCO_Util.NowS() + 300.0;
		m_mExclude.Set(id, ex);
		if (e.m_eType == DCO_EPlayerTaskType.ROLE && e.m_Objective)
			cmd.GetTactics().RefillRole(cmd, e.m_Objective, id, DCO_Util.NowS());
		Finish(ts, e, cmd);
		m_mTasks.Remove(id);
		DCO_Radio.Group(id, "COMMANDER", key, DCO_ERadioKind.INFO, DCO_Radio.P("callsign", o.m_sCallsign));
	}

	void AnswerOffer(AICommander_BaseComponent cmd, SCR_AIGroup grp, int offerId, bool accept, int pid)
	{
		if (!grp)
			return;
		DCO_PlayerOffer o = m_mOffer.Get(grp.GetGroupID());
		if (!o || o.m_iId != offerId)
			return;
		int result = DCO_EOfferResult.DECLINED;
		if (accept)
			result = DCO_EOfferResult.ACCEPTED;
		ResolveOffer(cmd, o, result, pid);
	}

	protected void CancelOffer(AICommander_BaseComponent cmd, DCO_PlayerTaskEntry e)
	{
		int oid = e.m_iOfferId;
		e.m_iOfferId = 0;
		DCO_PlayerOffer hit;
		foreach (int gid, DCO_PlayerOffer o : m_mOffer)
		{
			if (o.m_iId == oid)
			{
				hit = o;
				break;
			}
		}
		if (hit)
		{
			CloseOffer(cmd, hit, DCO_EOfferResult.CANCELLED);
			DCO_Radio.Group(hit.m_iGroup, "COMMANDER", "offer_cancelled", DCO_ERadioKind.INFO, DCO_Radio.P("callsign", hit.m_sCallsign));
			return;
		}
		foreach (int qid, array<ref DCO_PlayerOffer> q : m_mOfferQueue)
		{
			if (!q)
				continue;
			for (int i = q.Count() - 1; i >= 0; i--)
			{
				if (q[i].m_iId == oid)
					q.RemoveOrdered(i);
			}
		}
	}

	protected void ActivateWithoutOffer(SCR_TaskSystem ts, AICommander_BaseComponent cmd, int id, DCO_PlayerTaskEntry e)
	{
		if (e.m_iOfferId > 0)
			CancelOffer(cmd, e);
		e.m_bPending = false;
		if (!e.m_Task && ts)
			e.m_Task = CreateTask(ts, cmd, id, e);
	}

	protected void RecordFail(AICommander_BaseComponent cmd, int id, bool acceptance)
	{
		DCO_PlayerReliability rel = m_mReliability.Get(id);
		if (!rel)
		{
			rel = new DCO_PlayerReliability();
			m_mReliability.Insert(id, rel);
		}

		rel.m_iFails++;
		if (acceptance)
			rel.m_fCooldownUntil_s = DCO_Util.NowS() + 120.0;

		if (rel.m_iFails >= cmd.GetPlayerFailLimit())
		{
			rel.m_iFails = 0;
			rel.m_fBonusUntil_s = DCO_Util.NowS() + cmd.GetPlayerBonusMinutes() * 60;
			DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("group %1 failed %2x in a row -> bonus %3 min", id, cmd.GetPlayerFailLimit(), cmd.GetPlayerBonusMinutes()));
		}
	}

	protected void RecordSuccess(int id)
	{
		DCO_PlayerReliability rel = m_mReliability.Get(id);
		if (rel)
			rel.m_iFails = 0;
	}

	protected void ReleaseSlot(AICommander_BaseComponent cmd, DCO_PlayerTaskEntry e)
	{
		if (!e.m_bSlot || !cmd || !e.m_Objective)
			return;

		e.m_bSlot = false;
		FactionKey fk = cmd.GetCommanderFactionKey();
		if (e.m_Objective.GetCurrentAssignedGroupCount(fk) > 0)
			e.m_Objective.SetObjectiveGroup(fk, -1);
	}

	protected void Finish(SCR_TaskSystem ts, DCO_PlayerTaskEntry e, AICommander_BaseComponent cmd, bool success = false, bool keepSlot = false)
	{
		if (!e)
			return;

		if (e.m_iOfferId > 0)
			CancelOffer(cmd, e);

		if (!keepSlot)
			ReleaseSlot(cmd, e);
		if (!e.m_Task || !ts)
			return;

		SCR_ETaskState state = SCR_ETaskState.CANCELLED;
		if (success)
		{
			state = SCR_ETaskState.COMPLETED;
			e.m_bDone = true;
		}

		ts.SetTaskState(e.m_Task, state);
		GetGame().GetCallqueue().CallLater(DeleteTask, 10000, false, e.m_Task.GetTaskID());
		DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("%1 -> %2", e.m_Task.GetTaskID(), typename.EnumToString(SCR_ETaskState, state)));
	}

	protected void DeleteTask(string taskID)
	{
		SCR_TaskSystem ts = SCR_TaskSystem.GetInstance();
		SCR_Task task = SCR_TaskSystem.GetTaskFromTaskID(taskID, false);
		if (ts && task)
			ts.DeleteTask(task);
	}

	static vector LeaderPos(SCR_AIGroup grp)
	{
		IEntity leader = grp.GetLeaderEntity();
		if (leader)
			return leader.GetOrigin();
		return grp.GetOrigin();
	}

}
