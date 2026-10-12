modded class AICommander_BaseComponent
{
	void SetObjectiveCooldown(CMD_AICommanderObjectiveComponent obj, float seconds)
	{
		m_mAttackCooldown.Set(obj, DCO_Util.NowS() + seconds);
	}

	float GetMinAttackRatio()
	{
		DCO_Doctrine d = GetDoctrine();
		float r;
		if (d && d.m_fAttackRatio > 0)
			r = Math.Max(1, d.m_fAttackRatio * Math.Lerp(m_fRatioScaleCautious, m_fRatioScaleAggressive, Math.Clamp(DCO_EffTraitAggression() / 2, 0, 1)));
		else
			r = Math.Max(1, Math.Lerp(m_fAttackRatioCautious, m_fAttackRatioAggressive, DCO_EffAggression()));
		if (!DCO_AggProfile())
			return r;
		float floorR = Math.Min(r, DCO_Ctx.TuneForCmd(this, "agg.k2_ratio_floor", 1.3));
		if (DCO_K2())
			return Math.Max(r * Math.Pow(DCO_K2Slider(), -0.3), floorR);
		return Math.Max(r * Math.Pow(DCO_AggrA(), -0.3), Math.Min(r, 1.3));
	}

	int EstimateEnemyAt(CMD_AICommanderObjectiveComponent obj)
	{
		if (!threatComp || !obj)
			return 0;
		int raw;
		return Math.Round(m_Ops.Estimate(this, obj.GetOwner().GetOrigin(), obj.GetRadius() + m_fEnemyEstimateMargin, DCO_Util.NowS(), true, raw));
	}

	float GetAttackStrength(CMD_AICommanderObjectiveComponent obj)
	{
		float s;
		bool planned = m_Tactics && m_Tactics.GetPlan(obj);
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!g || g.GetGroupObjective() != obj)
				continue;
			if (!TaskCountsSlot(g.GetTask()) && !(planned && DCO_TacticsCompat.OwnsGroup(m_Tactics, g)))
				continue;
			float w = g.GetUnitCount();
			if (g.IsArmor())
				w *= m_fArmorStrengthWeight;
			s += w;
		}
		return s;
	}

	bool IsAttackRatioMet(CMD_AICommanderObjectiveComponent obj)
	{
		bool ok = m_Ops.IsForceSufficient(this, obj);
		if (ok || !DCO_IsForcedFocus(obj))
			return ok;
		int assigned = obj.GetCurrentAssignedGroupCount(m_sFactionKey);
		if (assigned < 2)
		{
			DCO_Perf.Count("obj_focus:force_min_squads");
			if (DCO_RoleOnce(obj.GetLogName() + "|force_min", 60.0))
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), string.Format("obj_focus_force cmd=%1 obj=%2 gate=min_squads assigned=%3 action=wait -- forced focus still needs 2 squads", m_sCommanderUID, obj.GetLogName(), assigned));
			return false;
		}
		DCO_Perf.Count("obj_focus:force_ratio");
		if (m_sDCOFocusRatioLog != obj.GetLogName())
		{
			m_sDCOFocusRatioLog = obj.GetLogName();
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, DCO_Log.IdCmd(this), string.Format("obj_focus_force cmd=%1 obj=%2 gate=ratio action=go strength=%3 enemy_est=%4 -- attacking below the target ratio because focus is forced", m_sCommanderUID, obj.GetLogName(), Math.Round(GetAttackStrength(obj)), EstimateEnemyAt(obj)));
		}
		return true;
	}

	int GetRequiredAttackGroups(CMD_AICommanderObjectiveComponent obj)
	{
		int floor = obj.GetRequiredGroupCount();
		DCO_TacticPlan plan = m_Tactics.GetPlan(obj);
		if (plan && plan.m_eTactic == DCO_ETactic.FIRE_AND_MANEUVER)
			floor = Math.Max(floor, 2);

		float total;
		int groups;
		foreach (DCO_GroupUtilityComponent g : m_aOwnedGroup)
		{
			if (!g || g.IsPlayerGroup() || g.IsDedicatedTransport() || g.IsMortar())
				continue;
			total += g.GetUnitCount();
			groups++;
		}
		float avg = m_fAssumedGroupSize;
		if (groups > 0)
			avg = Math.Max(total / groups, 2);

		return Math.Max(floor, m_Ops.RequiredGroups(this, obj, avg));
	}

	bool IsObjectiveOnCooldown(CMD_AICommanderObjectiveComponent obj)
	{
		float until;
		return m_mAttackCooldown.Find(obj, until) && DCO_Util.NowS() < until;
	}

	protected void DCO_ObjSkip(CMD_AICommanderObjectiveComponent obj, string reason, float worldTime)
	{
		DCO_Perf.Count("obj_skip:" + reason);
		float at;
		if (m_mDCOSkipLogAt.Find(obj, at) && worldTime - at < 60.0)
			return;
		m_mDCOSkipLogAt.Set(obj, worldTime);
		string line = string.Format("obj_skip cmd=%1 obj=%2 reason=%3 state=%4", m_sCommanderUID, obj.GetLogName(), reason,
			typename.EnumToString(CMD_EObjectiveState, obj.GetObjectiveState(m_sFactionKey)));
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(this), line);
	}

	protected bool DCO_HasActiveRecon(CMD_AICommanderObjectiveComponent obj)
	{
		DCO_GroupUtilityComponent r = obj.GetReconGroup(m_sFactionKey);
		return r && r.GetOwner() && r.GetGroupObjective() == obj && r.GetTask() == DCO_EGroupTask.RECON;
	}

	int DCO_ObjectiveRank(CMD_AICommanderObjectiveComponent obj)
	{
		return m_aObjective.Find(obj);
	}

	protected string DCO_TacticNameOf(DCO_TacticPlan plan)
	{
		if (!plan)
			return "none";
		return DCO_CommanderTactics.Name(plan.m_eTactic);
	}

	bool DCO_SyncDeadlinePassed(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		float deadline;
		return m_mSyncDeadline.Find(obj, deadline) && worldTime > deadline;
	}

	bool IsAssaultReleased(CMD_AICommanderObjectiveComponent obj)
	{
		bool released;
		return m_mAssaultReleased.Find(obj, released) && released;
	}

	bool GetStagingPos(CMD_AICommanderObjectiveComponent obj, out vector pos)
	{
		return m_mStagingPos.Find(obj, pos);
	}

	float GetAssaultCountdown(CMD_AICommanderObjectiveComponent obj)
	{
		if (IsAssaultReleased(obj))
			return 0;

		float first;
		if (!m_mFirstArrivalTime.Find(obj, first))
			return -1;

		float now = DCO_Util.NowS();
		return Math.Max(0, first + m_fSyncAttackMaxWaitTime - now);
	}

	string DescribeOperationGroups(CMD_AICommanderObjectiveComponent obj)
	{
		string s;
		vector objPos = obj.GetOwner().GetOrigin();
		foreach (DCO_GroupUtilityComponent grp : m_aOwnedGroup)
		{
			if (!grp || grp.GetGroupObjective() != obj || !TaskCountsSlot(grp.GetTask()))
				continue;

			if (!s.IsEmpty())
				s += ", ";
			string what = "assault";
			if (grp.GetTask() == DCO_EGroupTask.FLANK)
				what = "flank";
			s += string.Format("%1 (%2 from the %3)", DCO_PlayerComms.GetCallsign(SCR_AIGroup.Cast(grp.GetOwner())), what,
				DCO_PlayerComms.Bearing(objPos, grp.GetOwner().GetOrigin()));
		}
		return s;
	}

	protected void NotifyPlayersReleased(CMD_AICommanderObjectiveComponent obj)
	{
		if (m_bPlayerTasking && obj)
			m_PlayerTasking.OnAssaultReleased(this, obj);
	}

	bool GetPlayerTaskTarget(vector fromPos, out CMD_AICommanderObjectiveComponent outObj, out DCO_EPlayerTaskType outType, out vector outPos, array<CMD_AICommanderObjectiveComponent> skip = null)
	{
		float best = float.MAX;
		foreach (CMD_AICommanderObjectiveComponent obj : m_aObjective)
		{
			if (!obj || obj.IsCapturedBy(m_sFactionKey, m_sCommanderUID) || (skip && skip.Contains(obj)))
				continue;

			float d = vector.DistanceSq(fromPos, obj.GetOwner().GetOrigin());
			if (d < best)
			{
				best = d;
				outObj = obj;
			}
		}

		if (outObj)
		{
			bool released;
			vector staging;
			m_mAssaultReleased.Find(outObj, released);
			if (!released && m_mStagingStartTime.Contains(outObj) && m_mStagingPos.Find(outObj, staging))
			{
				outType = DCO_EPlayerTaskType.MOVE;
				outPos = staging;
			}
			else
			{
				outType = DCO_EPlayerTaskType.ATTACK;
				outPos = outObj.GetOwner().GetOrigin();
			}
			return true;
		}

		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return false;

		foreach (CMD_AICommanderObjectiveComponent own : mgr.m_aObjective)
		{
			if (!own || !own.IsCapturedBy(m_sFactionKey, m_sCommanderUID) || !IsObjectiveContested(own))
				continue;

			float d = vector.DistanceSq(fromPos, own.GetOwner().GetOrigin());
			if (d < best)
			{
				best = d;
				outObj = own;
			}
		}

		if (!outObj)
			return false;

		outType = DCO_EPlayerTaskType.DEFEND;
		outPos = outObj.GetOwner().GetOrigin();
		return true;
	}

	protected void RecordAssignedTime(CMD_AICommanderObjectiveComponent obj, float worldTime)
	{
		if (!m_mAssignedTime.Contains(obj))
			m_mAssignedTime.Insert(obj, worldTime);
		else
			m_mAssignedTime.Set(obj, worldTime);
	}
}
