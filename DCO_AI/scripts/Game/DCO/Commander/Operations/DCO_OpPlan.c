class DCO_OpObj
{
	CMD_AICommanderObjectiveComponent m_Obj;
	float m_fDef;
	float m_fDef0;
	int m_iNeed;
	int m_iValue;
	int m_iBand0;
	string m_sWhy;
	bool m_bQualify;
	ref array<CMD_AICommanderObjectiveComponent> m_aBlocks = {};
	ref array<CMD_AICommanderObjectiveComponent> m_aBlockedBy = {};
	vector m_vSbfEst;
	float m_fApproachMin = -1;
}

class DCO_OpPhase
{
	int m_iN;
	CMD_AICommanderObjectiveComponent m_Main;
	ref array<CMD_AICommanderObjectiveComponent> m_aCapture = {};
	ref array<CMD_AICommanderObjectiveComponent> m_aFix = {};
	float m_fEta;
	string m_sFires;
}

class DCO_Operation
{
	CMD_AICommanderObjectiveComponent m_Primary;
	ref array<ref DCO_OpObj> m_aObjs = {};
	ref array<ref DCO_OpPhase> m_aPhases = {};
	ref array<CMD_AICommanderObjectiveComponent> m_aDone = {};
	ref array<CMD_AICommanderObjectiveComponent> m_aToFix = {};
	ref map<CMD_AICommanderObjectiveComponent, int> m_mCapStart = new map<CMD_AICommanderObjectiveComponent, int>();
	ref map<CMD_AICommanderObjectiveComponent, float> m_mIntelSince = new map<CMD_AICommanderObjectiveComponent, float>();
	ref map<CMD_AICommanderObjectiveComponent, float> m_mLogAt = new map<CMD_AICommanderObjectiveComponent, float>();
	ref set<int> m_sThreatSeen = new set<int>();
	int m_iCur;
	float m_fStart;
	float m_fPhaseStart;
	float m_fProgressAt;
	float m_fBestLead = float.MAX;
	int m_iBestKnown = 9999;
	int m_iMainStart;
	int m_iStartUnits;
	float m_fConsolidateUntil;
	float m_fReserveAt;
	float m_fBranchSkipAt;
	float m_fBypassAt;
	string m_sSrc;
	string m_sDoctrine;
	vector m_vFpfTarget;
	CMD_AICommanderObjectiveComponent m_FpfObj;
	float m_fFpfUntil;
}

class DCO_NoFireAreas
{
	protected static ref array<vector> s_aPos;
	protected static ref array<DCO_GroupUtilityComponent> s_aGrp;
	protected static ref array<AICommander_BaseComponent> s_aCmd;
	protected static ref map<IEntity, float> s_mLogAt;

	static void Reset(AICommander_BaseComponent cmd)
	{
		if (!s_aPos)
		{
			s_aPos = {};
			s_aGrp = {};
			s_aCmd = {};
		}
		for (int i = s_aCmd.Count() - 1; i >= 0; i--)
		{
			if (s_aCmd[i] == cmd || !s_aCmd[i])
			{
				s_aPos.Remove(i);
				s_aGrp.Remove(i);
				s_aCmd.Remove(i);
			}
		}
	}

	static void Add(AICommander_BaseComponent cmd, DCO_GroupUtilityComponent g, vector pos)
	{
		if (!s_aPos)
			Reset(cmd);
		s_aPos.Insert(pos);
		s_aGrp.Insert(g);
		s_aCmd.Insert(cmd);
	}

	static IEntity Block(DCO_GroupUtilityComponent own, IEntity shooter, vector from, vector target)
	{
		if (!s_aPos || !own || !own.GetMyCommander())
			return null;
		AICommander_BaseComponent cmd = own.GetMyCommander();
		foreach (int i, vector c : s_aPos)
		{
			if (s_aCmd[i] != cmd || s_aGrp[i] == own || !s_aGrp[i] || !s_aGrp[i].GetOwner())
				continue;
			if (vector.DistanceXZ(from, c) <= 60.0)
				continue;
			vector ab = target - from;
			ab[1] = 0;
			vector ac = c - from;
			ac[1] = 0;
			float l2 = ab.LengthSq();
			float tt = 0;
			if (l2 > 0.01)
				tt = Math.Clamp(vector.Dot(ac, ab) / l2, 0, 1);
			if (vector.DistanceXZ(c, from + ab * tt) > 60.0)
				continue;
			AIGroup ag = AIGroup.Cast(s_aGrp[i].GetOwner());
			IEntity blocker = s_aGrp[i].GetOwner();
			if (ag && ag.GetLeaderEntity())
				blocker = ag.GetLeaderEntity();
			if (!s_mLogAt)
				s_mLogAt = new map<IEntity, float>();
			float now = DCO_Util.NowS();
			float at;
			if (!s_mLogAt.Find(shooter, at) || now - at >= 10.0)
			{
				s_mLogAt.Set(shooter, now);
				DCO_Perf.Count("op_nofire_reject");
				DCO_Log.Write(DCO_ELogChannel.TACTIC, DCO_ELogLevel.INFO, "-", string.Format("op_nofire_reject shooter=%1 target=%2 blocked_by=%3 dist=%4", shooter, DCO_Util.XZ(target), s_aGrp[i].GetOwner().GetName(), Math.Round(vector.DistanceXZ(target, c))));
			}
			return blocker;
		}
		return null;
	}
}

class DCO_OpPlanner
{
	protected ref DCO_Operation m_Op;
	protected CMD_AICommanderObjectiveComponent m_Main;
	protected static ref map<CMD_AICommanderObjectiveComponent, AICommander_BaseComponent> s_mClaim;
	protected string m_sLastPrimaryLog;

	bool HasOp()
	{
		return m_Op != null;
	}

	CMD_AICommanderObjectiveComponent Main()
	{
		return m_Main;
	}

	bool Contains(CMD_AICommanderObjectiveComponent o)
	{
		if (!m_Op || !o)
			return false;
		foreach (DCO_OpObj oo : m_Op.m_aObjs)
		{
			if (oo.m_Obj == o)
				return true;
		}
		return false;
	}

	protected void Ev(string s)
	{
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", s);
		DCO_Events.Operation(s);
	}

	protected bool Done(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent o)
	{
		if (!o || !o.IsCapturedBy(cmd.GetCommanderFactionKey()))
			return false;
		return true;
	}

	protected float Defenders(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent o)
	{
		float v = 0;
		DCO_DoctrineRuntime rt = cmd.GetDoctrineRuntime();
		if (rt)
		{
			DCO_ForceEval e = rt.GetEval(o);
			if (e)
				v = e.m_fEnemy;
		}
		if (cmd.GetOps())
			v = Math.Max(v, cmd.GetOps().EstimateDefenders(o));
		return v;
	}

	protected float Eff(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent o)
	{
		DCO_DoctrineRuntime rt = cmd.GetDoctrineRuntime();
		if (!rt)
			return 0;
		DCO_ForceEval e = rt.GetEval(o);
		if (!e)
			return 0;
		return e.m_fEff;
	}

	protected int Band(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent o)
	{
		DCO_DoctrineRuntime rt = cmd.GetDoctrineRuntime();
		if (!rt)
			return -1;
		return rt.GetBand(o);
	}

	protected int Available(AICommander_BaseComponent cmd)
	{
		int n = 0;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.IsPlayerGroup() || g.IsMortar() || g.IsDedicatedTransport() || g.IsMedicTeam() || g.GetUnitCount() <= 0 || g.m_DCOSupportOf)
				continue;
			DCO_EGroupTask t = g.GetTask();
			bool idle = t == DCO_EGroupTask.NONE || t == DCO_EGroupTask.PATROL;
			if (idle || Contains(g.GetGroupObjective()))
				n++;
		}
		return n;
	}

	protected int PlanNeed(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent o, float def)
	{
		int force = DCO_Force.AttackMax(cmd);
		float total = 0;
		int groups = 0;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || g.IsPlayerGroup() || g.IsDedicatedTransport() || g.IsMortar() || g.GetUnitCount() <= 0)
				continue;
			total += g.GetUnitCount();
			groups++;
		}
		float avg = 6.0;
		if (groups > 0)
			avg = Math.Max(total / groups, 2.0);
		float ratio = 2.0;
		if (cmd.GetOps())
			ratio = cmd.GetOps().GetTargetRatio(cmd, o);
		int need = Math.Ceil(def * ratio / avg);
		return Math.ClampInt(need, 1, force);
	}

	int NeedOf(CMD_AICommanderObjectiveComponent o)
	{
		if (!m_Op)
			return 0;
		DCO_OpObj oo = ObjOf(m_Op, o);
		if (!oo)
			return 0;
		return oo.m_iNeed;
	}

	protected int UnitsOn(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent o)
	{
		int n = 0;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (g && g.GetGroupObjective() == o && !g.IsPlayerGroup())
				n += g.GetUnitCount();
		}
		return n;
	}

	protected float SegDist(vector p, vector a, vector b)
	{
		vector ab = b - a;
		ab[1] = 0;
		vector ap = p - a;
		ap[1] = 0;
		float l2 = ab.LengthSq();
		if (l2 < 0.01)
			return vector.DistanceXZ(p, a);
		float t = Math.Clamp(vector.Dot(ap, ab) / l2, 0, 1);
		return vector.DistanceXZ(p, a + ab * t);
	}

	protected bool Los(vector a, vector b)
	{
		float len = vector.Distance(a, b);
		return len * (1.0 - DCO_Util.TraceFrac(a, b, null, TraceFlags.WORLD | TraceFlags.ENTS, -1, "t:op_plan")) <= 10.0;
	}

	protected CMD_AICommanderObjectiveComponent FindByName(string name)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr)
			return null;
		foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
		{
			if (o && o.GetOwner() && (o.GetOwner().GetName() == name || o.GetLogName() == name))
				return o;
		}
		return null;
	}

	protected void Secondaries(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent prim, notnull array<CMD_AICommanderObjectiveComponent> outL, out string src, out float radius, out string linkedText)
	{
		FactionKey fk = cmd.GetCommanderFactionKey();
		array<string> linked = prim.DCO_LinkedSecondaries();
		radius = prim.DCO_SecondaryRadius();
		linkedText = "-";
		array<CMD_AICommanderObjectiveComponent> cand = {};
		if (linked && !linked.IsEmpty())
		{
			src = "linked";
			linkedText = "";
			foreach (string ln : linked)
			{
				if (!linkedText.IsEmpty())
					linkedText += "|";
				linkedText += ln;
				CMD_AICommanderObjectiveComponent lo = FindByName(ln);
				if (!lo)
				{
					Ev(string.Format("op_secondary_skip cmd=%1 obj=%2 reason=missing primary=%3", cmd.GetCommanderUID(), ln, prim.GetLogName()));
					continue;
				}
				cand.Insert(lo);
			}
		}
		else
		{
			src = "radius";
			if (radius <= 0 && DCO_TacticTest.ScopeActive(cmd))
			{
				radius = DCO_TacticTest.ScopeRadius();
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.WARN, "-", string.Format("op_plan_warn cmd=%1 primary=%2 reason=no_radius_no_links fallback_radius=%3", cmd.GetCommanderUID(), prim.GetLogName(), Math.Round(radius)));
			}
			AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
			if (mgr)
			{
				foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
				{
					if (o && o != prim && o.GetOwner() && vector.DistanceXZ(o.GetOwner().GetOrigin(), prim.GetOwner().GetOrigin()) <= radius)
						cand.Insert(o);
				}
			}
		}
		if (!s_mClaim)
			s_mClaim = new map<CMD_AICommanderObjectiveComponent, AICommander_BaseComponent>();
		foreach (CMD_AICommanderObjectiveComponent c : cand)
		{
			if (c == prim || outL.Contains(c))
				continue;
			if (cmd.DCO_AssignSkip(c, "op_secondary") || cmd.DCO_RoleBlocksAttack(c, "op_secondary") || cmd.DCO_PhaseSkip(c, "op_secondary"))
			{
				Ev(string.Format("op_secondary_skip cmd=%1 obj=%2 reason=not_assigned primary=%3", cmd.GetCommanderUID(), c.GetLogName(), prim.GetLogName()));
				continue;
			}
			if (c.IsCapturedBy(fk))
			{
				Ev(string.Format("op_secondary_skip cmd=%1 obj=%2 reason=held primary=%3", cmd.GetCommanderUID(), c.GetLogName(), prim.GetLogName()));
				continue;
			}
			AICommander_BaseComponent owner;
			if (s_mClaim.Find(c, owner) && owner && owner != cmd && owner.GetCommanderFactionKey() == fk)
			{
				Ev(string.Format("op_secondary_skip cmd=%1 obj=%2 reason=other_op primary=%3", cmd.GetCommanderUID(), c.GetLogName(), prim.GetLogName()));
				continue;
			}
			outL.Insert(c);
		}
	}

	protected CMD_AICommanderObjectiveComponent Anchor(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent pick)
	{
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!mgr || !pick)
			return pick;
		FactionKey fk = cmd.GetCommanderFactionKey();
		CMD_AICommanderObjectiveComponent best;
		float bestD = float.MAX;
		foreach (CMD_AICommanderObjectiveComponent p : mgr.m_aObjective)
		{
			if (!p || p == pick || !p.DCO_IsPrimary() || p.IsCapturedBy(fk) || !p.GetOwner() || !cmd.DCO_MayAttack(p))
				continue;
			bool linked = false;
			array<string> names = p.DCO_LinkedSecondaries();
			if (names && !names.IsEmpty())
			{
				foreach (string n : names)
				{
					if (FindByName(n) == pick)
						linked = true;
				}
			}
			else if (vector.DistanceXZ(p.GetOwner().GetOrigin(), pick.GetOwner().GetOrigin()) <= p.DCO_SecondaryRadius())
				linked = true;
			if (!linked)
				continue;
			float d = vector.DistanceXZ(p.GetOwner().GetOrigin(), pick.GetOwner().GetOrigin());
			if (d < bestD)
			{
				bestD = d;
				best = p;
			}
		}
		if (best)
			return best;
		return pick;
	}

	void Apply(AICommander_BaseComponent cmd, notnull array<CMD_AICommanderObjectiveComponent> list, float now)
	{
		if (!Replication.IsServer())
			return;
		bool test = DCO_TacticTest.ScopeActive(cmd);
		if (DCO_TacticTest.ForCommander(cmd) && !test)
			return;
		CMD_AICommanderObjectiveComponent prim;
		CMD_AICommanderObjectiveComponent focus = cmd.DCO_GetFocus();
		if (test)
			prim = DCO_TacticTest.Primary(cmd);
		else if (focus && !Done(cmd, focus) && !cmd.IsObjectiveOnCooldown(focus) && cmd.DCO_MayAttack(focus))
			prim = focus;
		else if (m_Op && m_Op.m_Primary && !Done(cmd, m_Op.m_Primary) && cmd.DCO_MayAttack(m_Op.m_Primary))
			prim = m_Op.m_Primary;
		else
		{
			if (m_Op && m_Op.m_Primary && !Done(cmd, m_Op.m_Primary))
				EndOp(cmd, "not_assigned");
			foreach (CMD_AICommanderObjectiveComponent c : list)
			{
				if (c && !c.IsCapturedBy(cmd.GetCommanderFactionKey()) && cmd.DCO_MayAttack(c))
				{
					prim = Anchor(cmd, c);
					break;
				}
			}
		}
		if (!prim)
		{
			if (m_Op)
				EndOp(cmd, "no_primary");
			return;
		}
		if (!m_Op || m_Op.m_Primary != prim)
			Plan(cmd, prim, now, "new");
		if (!m_Op)
			return;
		Triggers(cmd, now);
		if (!m_Op)
			return;
		Execute(cmd, list, now);
	}

	protected void EndOp(AICommander_BaseComponent cmd, string why)
	{
		if (!m_Op)
			return;
		Ev(string.Format("op_end cmd=%1 primary=%2 reason=%3 phases=%4 done=%5 dur_s=%6", cmd.GetCommanderUID(), m_Op.m_Primary.GetLogName(), why, m_Op.m_aPhases.Count(), m_Op.m_aDone.Count(), Math.Round(DCO_Util.NowS() - m_Op.m_fStart)));
		if (s_mClaim)
		{
			foreach (DCO_OpObj oo : m_Op.m_aObjs)
			{
				AICommander_BaseComponent owner;
				if (s_mClaim.Find(oo.m_Obj, owner) && owner == cmd)
					s_mClaim.Remove(oo.m_Obj);
			}
		}
		m_Op = null;
		m_Main = null;
		DCO_NoFireAreas.Reset(cmd);
	}

	protected void Plan(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent prim, float now, string why)
	{
		CMD_AICommanderObjectiveComponent keepMain;
		array<CMD_AICommanderObjectiveComponent> done = {};
		if (m_Op && m_Op.m_Primary == prim)
		{
			done.InsertAll(m_Op.m_aDone);
			DCO_OpPhase cur = CurPhase();
			if (cur && cur.m_Main && MainInContact(cmd, cur.m_Main))
			{
				keepMain = cur.m_Main;
				DCO_Perf.Count("op:replan_skip_close_contact");
				Ev(string.Format("op_replan_skip cmd=%1 reason=close_contact main=%2 action=keep_main", cmd.GetCommanderUID(), keepMain.GetLogName()));
			}
		}
		else if (m_Op)
			EndOp(cmd, "primary_changed");
		DCO_Operation op = new DCO_Operation();
		op.m_Primary = prim;
		op.m_fStart = now;
		op.m_aDone.InsertAll(done);
		string fk = cmd.GetCommanderFactionKey();
		op.m_sDoctrine = "us_shaping";
		if (fk.StartsWith("USSR") || fk.StartsWith("RU"))
			op.m_sDoctrine = "ussr_depth";
		array<CMD_AICommanderObjectiveComponent> secs = {};
		float radius;
		string linkedText;
		string src;
		Secondaries(cmd, prim, secs, src, radius, linkedText);
		if (prim == cmd.DCO_GetFocus() && !secs.IsEmpty() && DCO_Ctx.TuneForCmd(cmd, "obj.focus_shaping", 0) <= 0)
		{
			Ev(string.Format("op_focus_no_shaping cmd=%1 primary=%2 dropped=%3", cmd.GetCommanderUID(), prim.GetLogName(), secs.Count()));
			secs.Clear();
		}
		op.m_sSrc = src;
		if (!s_mClaim)
			s_mClaim = new map<CMD_AICommanderObjectiveComponent, AICommander_BaseComponent>();
		vector centroid = cmd.DCO_AttackerCentroid();
		vector primPos = prim.GetOwner().GetOrigin();
		vector staging = centroid;
		vector sp;
		if (cmd.GetStagingPos(prim, sp) && sp != vector.Zero)
			staging = sp;
		float defP = Defenders(cmd, prim);
		int needP = PlanNeed(cmd, prim, defP);
		array<CMD_AICommanderObjectiveComponent> all = {};
		all.Insert(prim);
		all.InsertAll(secs);
		string secText = "";
		foreach (CMD_AICommanderObjectiveComponent o : all)
		{
			if (done.Contains(o) || Done(cmd, o))
				continue;
			DCO_OpObj oo = new DCO_OpObj();
			oo.m_Obj = o;
			oo.m_fDef = Defenders(cmd, o);
			oo.m_fDef0 = oo.m_fDef;
			oo.m_iNeed = PlanNeed(cmd, o, oo.m_fDef);
			oo.m_iBand0 = Band(cmd, o);
			vector xo = o.GetOwner().GetOrigin();
			vector toSt = DCO_Specialist.Flat(staging - xo);
			oo.m_vSbfEst = DCO_Util.Surface(xo + toSt * (o.GetRadius() + 250.0));
			s_mClaim.Set(o, cmd);
			op.m_aObjs.Insert(oo);
		}
		foreach (DCO_OpObj bx : op.m_aObjs)
		{
			vector xp = bx.m_Obj.GetOwner().GetOrigin();
			float xd = vector.DistanceXZ(xp, staging);
			foreach (DCO_OpObj yo : op.m_aObjs)
			{
				if (yo == bx || yo.m_Obj == prim)
					continue;
				vector yp = yo.m_Obj.GetOwner().GetOrigin();
				if (vector.DistanceXZ(yp, staging) >= xd)
					continue;
				float reach = yo.m_Obj.GetRadius() + 200.0;
				float lineD = SegDist(yp, staging, xp);
				float sbfD = vector.DistanceXZ(yp, bx.m_vSbfEst);
				float nearD = Math.Min(lineD, sbfD);
				if (bx.m_fApproachMin < 0 || nearD < bx.m_fApproachMin)
					bx.m_fApproachMin = nearD;
				if (lineD <= reach || sbfD <= reach)
				{
					yo.m_aBlocks.Insert(bx.m_Obj);
					bx.m_aBlockedBy.Insert(yo.m_Obj);
				}
			}
		}
		foreach (DCO_OpObj oo2 : op.m_aObjs)
		{
			CMD_AICommanderObjectiveComponent o2 = oo2.m_Obj;
			string w = "";
			if (o2 == prim)
			{
				oo2.m_iValue = 3;
				w = "primary";
			}
			else
			{
				vector op2 = o2.GetOwner().GetOrigin();
				oo2.m_iValue = 1;
				if (oo2.m_fDef <= defP * 0.5)
				{
					w += "easy";
					oo2.m_bQualify = true;
				}
				float sbfDist = vector.DistanceXZ(op2, primPos);
				bool sbfLos = Los(op2 + Vector(0, 2.0, 0), primPos + Vector(0, 1.5, 0));
				bool sbfSide = cmd.DCO_FriendlySide(primPos, prim.GetRadius(), op2);
				if (sbfDist <= 450.0 && sbfLos && sbfSide)
				{
					oo2.m_iValue++;
					oo2.m_bQualify = true;
					if (!w.IsEmpty())
						w += "+";
					w += "sbf_site";
				}
				int lanes = 0;
				for (int k = 1; k <= 3; k++)
				{
					vector lp = staging + (primPos - staging) * (1.0 * k * 0.25);
					if (Los(op2 + Vector(0, 2.0, 0), DCO_Util.Surface(lp) + Vector(0, 1.5, 0)))
						lanes++;
				}
				if (lanes > 0)
				{
					oo2.m_iValue++;
					oo2.m_bQualify = true;
					if (!w.IsEmpty())
						w += "+";
					w += "flank";
				}
				Ev(string.Format("op_value cmd=%1 obj=%2 def=%3 def_primary=%4 easy=%5 sbf_site=los:%6,dist:%7,side:%8 flank=%9/3", cmd.GetCommanderUID(), o2.GetLogName(), Math.Round(oo2.m_fDef), Math.Round(defP), DCO_Util.B01(oo2.m_fDef <= defP * 0.5), DCO_Util.B01(sbfLos), Math.Round(sbfDist), DCO_Util.B01(sbfSide), lanes)
					+ string.Format(" blocks=%1 blocked_by=%2 sbf_est=%3 approach_min=%4 staging=%5 need=%6", Names(oo2.m_aBlocks), Names(oo2.m_aBlockedBy), DCO_Util.XZ(oo2.m_vSbfEst), Math.Round(oo2.m_fApproachMin), DCO_Util.XZ(staging), oo2.m_iNeed));
			}
			if (!oo2.m_aBlocks.IsEmpty())
			{
				oo2.m_iValue += oo2.m_aBlocks.Count();
				oo2.m_bQualify = true;
				string bl = "";
				foreach (CMD_AICommanderObjectiveComponent bo : oo2.m_aBlocks)
				{
					if (!bl.IsEmpty())
						bl += "|";
					bl += bo.GetLogName();
				}
				if (!w.IsEmpty())
					w += "+";
				w += "blocks:" + bl;
			}
			if (w.IsEmpty())
				w = "fix";
			oo2.m_sWhy = w;
			if (!secText.IsEmpty())
				secText += ",";
			secText += string.Format("%1:%2:%3:%4", o2.GetLogName(), Math.Round(oo2.m_fDef), oo2.m_iNeed, oo2.m_iValue);
		}
		array<CMD_AICommanderObjectiveComponent> order = {};
		array<float> dist = {};
		array<CMD_AICommanderObjectiveComponent> late = {};
		array<float> lateD = {};
		foreach (DCO_OpObj ob : op.m_aObjs)
		{
			float d = vector.DistanceXZ(ob.m_Obj.GetOwner().GetOrigin(), centroid);
			bool first = op.m_sDoctrine == "ussr_depth" || ob.m_bQualify;
			if (ob.m_Obj == prim && op.m_sDoctrine != "ussr_depth")
				continue;
			if (first)
				InsertSorted(order, dist, ob.m_Obj, d);
			else
				InsertSorted(late, lateD, ob.m_Obj, d);
		}
		if (op.m_sDoctrine != "ussr_depth")
			order.Insert(prim);
		order.InsertAll(late);
		for (int pass = 0; pass < 10; pass++)
		{
			bool changed = false;
			foreach (DCO_OpObj ob2 : op.m_aObjs)
			{
				int yi = order.Find(ob2.m_Obj);
				if (yi < 0)
					continue;
				int firstX = -1;
				CMD_AICommanderObjectiveComponent firstObj;
				foreach (CMD_AICommanderObjectiveComponent xb : ob2.m_aBlocks)
				{
					int xi = order.Find(xb);
					if (xi >= 0 && xi < yi && (firstX < 0 || xi < firstX))
					{
						firstX = xi;
						firstObj = xb;
					}
				}
				if (firstX < 0)
					continue;
				order.Remove(yi);
				order.InsertAt(ob2.m_Obj, firstX);
				changed = true;
				DCO_Perf.Count("op:order_fix");
				Ev(string.Format("op_order_fix cmd=%1 moved=%2 before=%3 reason=blocks", cmd.GetCommanderUID(), ob2.m_Obj.GetLogName(), firstObj.GetLogName()));
			}
			if (!changed)
				break;
		}
		if (keepMain && order.Contains(keepMain))
		{
			order.RemoveItem(keepMain);
			order.InsertAt(keepMain, 0);
		}
		int avail = Available(cmd);
		int reserve = DCO_TacticTest.ReserveGroups(cmd, avail);
		string orderText = "";
		foreach (int i, CMD_AICommanderObjectiveComponent mo : order)
		{
			DCO_OpPhase ph = new DCO_OpPhase();
			ph.m_iN = i + 1;
			ph.m_Main = mo;
			int remaining = avail - reserve - Math.Ceil(1.0 * NeedOfOp(op, mo) * 1.2);
			for (int j = i + 1; j < order.Count(); j++)
			{
				int needJ = Math.Ceil(1.0 * NeedOfOp(op, order[j]) * 1.2);
				if (ph.m_aCapture.IsEmpty() && needJ <= remaining)
				{
					ph.m_aCapture.Insert(order[j]);
					remaining -= needJ;
				}
				else
					ph.m_aFix.Insert(order[j]);
			}
			DCO_OpObj mobj = ObjOf(op, mo);
			float def = 0;
			if (mobj)
				def = mobj.m_fDef;
			ph.m_fEta = 240.0 + def * 20.0;
			ph.m_sFires = string.Format("prep:%1 fix:%2", mo.GetLogName(), ph.m_aFix.Count());
			op.m_aPhases.Insert(ph);
			if (!orderText.IsEmpty())
				orderText += ",";
			orderText += mo.GetLogName();
		}
		if (secText.IsEmpty())
			secText = "-";
		op.m_iCur = 0;
		op.m_fPhaseStart = now;
		op.m_fProgressAt = now;
		op.m_iStartUnits = 0;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (g && !g.IsPlayerGroup())
				op.m_iStartUnits += g.GetUnitCount();
		}
		m_Op = op;
		if (why == "new")
			DCO_Perf.Count("op:plan");
		string whyText = "";
		foreach (DCO_OpObj ow : op.m_aObjs)
		{
			if (ow.m_Obj == prim)
				continue;
			if (!whyText.IsEmpty())
				whyText += ",";
			whyText += ow.m_Obj.GetLogName() + ":" + ow.m_sWhy;
		}
		if (whyText.IsEmpty())
			whyText = "-";
		Ev(string.Format("op_plan cmd=%1 primary=%2 secondaries=%3 phases=%4 order=%5 forces=%6 reserve=%7 why=%8 doctrine_order=%9", cmd.GetCommanderUID(), prim.GetLogName(), secText, op.m_aPhases.Count(), orderText, avail, reserve, whyText, op.m_sDoctrine)
			+ string.Format(" secondaries_src=%1 radius=%2 linked=%3 trigger=%4", src, Math.Round(radius), linkedText, why));
		PhaseStart(cmd, now);
	}

	protected void InsertSorted(array<CMD_AICommanderObjectiveComponent> objs, array<float> keys, CMD_AICommanderObjectiveComponent o, float k)
	{
		int at = 0;
		while (at < keys.Count() && keys[at] <= k)
			at++;
		keys.InsertAt(k, at);
		objs.InsertAt(o, at);
	}

	protected string m_sNeutralLog;

	protected bool EmptyNeutral(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent o)
	{
		if (!o || !o.GetOwner() || !o.GetOwnerFaction().IsEmpty() || !cmd.GetOps())
			return false;
		return cmd.GetOps().PerceivedNear(o.GetOwner().GetOrigin(), o.GetRadius() + 150.0) == 0;
	}

	protected CMD_AICommanderObjectiveComponent BlockerOf(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent o)
	{
		DCO_OpObj oo = ObjOf(m_Op, o);
		if (!oo)
			return null;
		foreach (CMD_AICommanderObjectiveComponent b : oo.m_aBlockedBy)
		{
			if (!b || Done(cmd, b) || m_Op.m_aDone.Contains(b))
				continue;
			if (EmptyNeutral(cmd, b))
			{
				DCO_Perf.Count("op:blocker_neutral");
				if (m_sNeutralLog != b.GetLogName())
				{
					m_sNeutralLog = b.GetLogName();
					Ev(string.Format("op_blocker_neutral cmd=%1 obj=%2 blocker=%3 -- blocker is neutral with no enemy seen; not blocking", cmd.GetCommanderUID(), o.GetLogName(), b.GetLogName()));
				}
				continue;
			}
			return b;
		}
		return null;
	}

	protected int NeedOfOp(DCO_Operation op, CMD_AICommanderObjectiveComponent o)
	{
		DCO_OpObj oo = ObjOf(op, o);
		if (!oo)
			return 1;
		return oo.m_iNeed;
	}

	protected DCO_OpObj ObjOf(DCO_Operation op, CMD_AICommanderObjectiveComponent o)
	{
		foreach (DCO_OpObj oo : op.m_aObjs)
		{
			if (oo.m_Obj == o)
				return oo;
		}
		return null;
	}

	protected DCO_OpPhase CurPhase()
	{
		if (!m_Op || m_Op.m_iCur < 0 || m_Op.m_iCur >= m_Op.m_aPhases.Count())
			return null;
		return m_Op.m_aPhases[m_Op.m_iCur];
	}

	protected string Names(array<CMD_AICommanderObjectiveComponent> l)
	{
		string t = "";
		foreach (CMD_AICommanderObjectiveComponent o : l)
		{
			if (!o)
				continue;
			if (!t.IsEmpty())
				t += ",";
			t += o.GetLogName();
		}
		if (t.IsEmpty())
			return "-";
		return t;
	}

	protected void PhaseStart(AICommander_BaseComponent cmd, float now)
	{
		DCO_OpPhase ph = CurPhase();
		if (!ph)
			return;
		m_Op.m_fPhaseStart = now;
		m_Op.m_fProgressAt = now;
		m_Op.m_fBestLead = float.MAX;
		m_Op.m_iBestKnown = 9999;
		m_Op.m_iMainStart = UnitsOn(cmd, ph.m_Main);
		m_Op.m_mCapStart.Clear();
		m_Op.m_aToFix.Clear();
		foreach (CMD_AICommanderObjectiveComponent c : ph.m_aCapture)
			m_Op.m_mCapStart.Set(c, UnitsOn(cmd, c));
		DCO_Perf.Count("op:phase");
		Ev(string.Format("op_phase cmd=%1 n=%2 main=%3 capture=%4 fix=%5 fires=%6 eta_s=%7", cmd.GetCommanderUID(), ph.m_iN, ph.m_Main.GetLogName(), Names(ph.m_aCapture), Names(ph.m_aFix), ph.m_sFires, Math.Round(ph.m_fEta)));
	}

	protected bool MainInContact(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent o)
	{
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (g && g.GetGroupObjective() == o && g.IsInContact())
				return true;
		}
		return false;
	}

	protected void Triggers(AICommander_BaseComponent cmd, float now)
	{
		DCO_OpPhase ph = CurPhase();
		if (!ph)
		{
			EndOp(cmd, "all_phases_done");
			return;
		}
		if (now < m_Op.m_fConsolidateUntil)
			return;
		if (Done(cmd, ph.m_Main))
		{
			PhaseEnd(cmd, ph, "cleared", now);
			return;
		}
		Progress(cmd, ph, now);
		int stallS = cmd.DCO_OpStallS();
		bool culminated = !cmd.GetTactics().GetPlan(ph.m_Main) && now - m_Op.m_fPhaseStart > 60.0 && !ph.m_Main.IsCapturedBy(cmd.GetCommanderFactionKey()) && MainHadPlan(cmd, ph.m_Main);
		if ((now - m_Op.m_fProgressAt >= stallS && now - m_Op.m_fPhaseStart >= stallS) || culminated)
		{
			string sr = "stalled";
			if (culminated)
				sr = "culminated";
			TryShift(cmd, ph, now, sr);
		}
		Losses(cmd, ph, now);
		Intel(cmd, now);
		ReserveBlock(cmd, now);
		Fpf(cmd, now);
		Bypass(cmd, ph, now);
	}

	protected ref set<CMD_AICommanderObjectiveComponent> m_sPlanSeen = new set<CMD_AICommanderObjectiveComponent>();

	protected bool MainHadPlan(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent o)
	{
		return m_sPlanSeen.Contains(o);
	}

	protected void Progress(AICommander_BaseComponent cmd, DCO_OpPhase ph, float now)
	{
		if (cmd.GetTactics() && cmd.GetTactics().GetPlan(ph.m_Main))
			m_sPlanSeen.Insert(ph.m_Main);
		vector objPos = ph.m_Main.GetOwner().GetOrigin();
		float r = ph.m_Main.GetRadius() + 25.0;
		int known = 0;
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (tr)
		{
			foreach (CMD_ThreatEntry t : tr.GetThreats())
			{
				if (t && now - t.m_fLastUpdateTime <= 60.0 && vector.DistanceXZ(t.m_vBelievedPos, objPos) <= r)
					known += Math.Max(1, t.m_iEstimatedEnemyCount);
			}
		}
		float lead = float.MAX;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (g && g.GetGroupObjective() == ph.m_Main && g.GetTask() == DCO_EGroupTask.ATTACK && g.GetOwner())
				lead = Math.Min(lead, vector.DistanceXZ(g.GetOwner().GetOrigin(), objPos));
		}
		if (known < m_Op.m_iBestKnown)
		{
			if (m_Op.m_iBestKnown != 9999)
				m_Op.m_fProgressAt = now;
			m_Op.m_iBestKnown = known;
		}
		if (lead < m_Op.m_fBestLead - 30.0)
		{
			if (m_Op.m_fBestLead != float.MAX)
				m_Op.m_fProgressAt = now;
			m_Op.m_fBestLead = lead;
		}
	}

	protected void PhaseEnd(AICommander_BaseComponent cmd, DCO_OpPhase ph, string result, float now)
	{
		int losses = Math.Max(0, m_Op.m_iMainStart - UnitsOn(cmd, ph.m_Main));
		DCO_Perf.Count("op:phase_end_" + result);
		Ev(string.Format("op_phase_end cmd=%1 n=%2 main=%3 result=%4 dur_s=%5 losses=%6", cmd.GetCommanderUID(), ph.m_iN, ph.m_Main.GetLogName(), result, Math.Round(now - m_Op.m_fPhaseStart), losses));
		if (result == "cleared")
		{
			m_Op.m_aDone.Insert(ph.m_Main);
			RegisterFpf(cmd, ph.m_Main, now);
		}
		foreach (CMD_AICommanderObjectiveComponent c : ph.m_aCapture)
		{
			if (Done(cmd, c) && !m_Op.m_aDone.Contains(c))
				m_Op.m_aDone.Insert(c);
		}
		int next = m_Op.m_iCur + 1;
		while (next < m_Op.m_aPhases.Count() && (m_Op.m_aDone.Contains(m_Op.m_aPhases[next].m_Main) || Done(cmd, m_Op.m_aPhases[next].m_Main)))
			next++;
		m_Op.m_iCur = next;
		if (next >= m_Op.m_aPhases.Count())
		{
			EndOp(cmd, "all_phases_done");
			return;
		}
		DCO_Perf.Count("op:replan_phase_end");
		Ev(string.Format("op_replan cmd=%1 reason=phase_end next=%2", cmd.GetCommanderUID(), m_Op.m_aPhases[next].m_Main.GetLogName()));
		if (result == "cleared")
		{
			m_Op.m_fConsolidateUntil = now + 45.0;
			Ev(string.Format("op_consolidate cmd=%1 n=%2 s=45 obj=%3 face=%4", cmd.GetCommanderUID(), ph.m_iN, ph.m_Main.GetLogName(), m_Op.m_aPhases[next].m_Main.GetLogName()));
			ReserveExploit(cmd, m_Op.m_aPhases[next].m_Main, now);
		}
		PhaseStart(cmd, now);
	}

	protected void TryShift(AICommander_BaseComponent cmd, DCO_OpPhase ph, float now, string why)
	{
		if (now < m_Op.m_fBranchSkipAt)
			return;
		float effMain = Eff(cmd, ph.m_Main);
		CMD_AICommanderObjectiveComponent best;
		float bestEff = effMain;
		foreach (CMD_AICommanderObjectiveComponent f : ph.m_aFix)
		{
			if (!f || Done(cmd, f))
				continue;
			CMD_AICommanderObjectiveComponent blocker = BlockerOf(cmd, f);
			if (blocker)
			{
				DCO_Perf.Count("op:branch_skip_blocked");
				Ev(string.Format("op_branch_skip cmd=%1 from=%2 to=%3 reason=blocked_by=%4 trigger=%5", cmd.GetCommanderUID(), ph.m_Main.GetLogName(), f.GetLogName(), blocker.GetLogName(), why));
				continue;
			}
			float e = Eff(cmd, f);
			if (e > bestEff)
			{
				bestEff = e;
				best = f;
			}
		}
		if (!best)
		{
			m_Op.m_fBranchSkipAt = now + 120.0;
			DCO_Perf.Count("op:branch_skip_no_better");
			Ev(string.Format("op_branch_skip cmd=%1 from=%2 reason=no_better eff_main=%3 trigger=%4", cmd.GetCommanderUID(), ph.m_Main.GetLogName(), effMain.ToString(-1, 2), why));
			string action = "none";
			m_Op.m_fProgressAt = now;
			DCO_Perf.Count("op:stall_action_" + action);
			Ev(string.Format("op_stall_action cmd=%1 obj=%2 action=%3 trigger=%4", cmd.GetCommanderUID(), ph.m_Main.GetLogName(), action, why));
			return;
		}
		vector from = cmd.DCO_AttackerCentroid();
		vector to = best.GetOwner().GetOrigin();
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || !g.GetOwner() || g.GetTask() != DCO_EGroupTask.SUPPORT_BY_FIRE || !g.GetGroupObjective() || g.GetGroupObjective() == best)
				continue;
			vector gp = g.GetOwner().GetOrigin();
			vector gt = g.GetGroupObjective().GetOwner().GetOrigin();
			if (SegDist(gp, from, to) <= 50.0 || SegDist(from + (to - from) * 0.5, gp, gt) <= 30.0)
			{
				m_Op.m_fBranchSkipAt = now + 120.0;
				DCO_Perf.Count("op:branch_skip_route");
				Ev(string.Format("op_branch_skip cmd=%1 from=%2 to=%3 reason=route_crosses_fire fire_grp=%4 trigger=%5", cmd.GetCommanderUID(), ph.m_Main.GetLogName(), best.GetLogName(), g.GetOwner().GetName(), why));
				return;
			}
		}
		string reason = "main_shift";
		if (m_Op.m_sDoctrine == "ussr_depth")
			reason = "second_echelon_shift";
		DCO_Perf.Count("op:branch_" + reason);
		Ev(string.Format("op_phase_end cmd=%1 n=%2 main=%3 result=%4 dur_s=%5 losses=%6", cmd.GetCommanderUID(), ph.m_iN, ph.m_Main.GetLogName(), why, Math.Round(now - m_Op.m_fPhaseStart), Math.Max(0, m_Op.m_iMainStart - UnitsOn(cmd, ph.m_Main))));
		Ev(string.Format("op_branch cmd=%1 from=%2 to=%3 reason=%4 trigger=%5 eff_from=%6 eff_to=%7", cmd.GetCommanderUID(), ph.m_Main.GetLogName(), best.GetLogName(), reason, why, effMain.ToString(-1, 2), bestEff.ToString(-1, 2)));
		CMD_AICommanderObjectiveComponent old = ph.m_Main;
		ph.m_aFix.RemoveItem(best);
		ph.m_aFix.Insert(old);
		ph.m_Main = best;
		ph.m_sFires = string.Format("prep:%1 fix:%2", best.GetLogName(), ph.m_aFix.Count());
		PhaseStart(cmd, now);
	}

	protected void Losses(AICommander_BaseComponent cmd, DCO_OpPhase ph, float now)
	{
		float thr = cmd.DCO_OpLossPct() / 100.0;
		foreach (CMD_AICommanderObjectiveComponent c : ph.m_aCapture)
		{
			if (m_Op.m_aToFix.Contains(c))
				continue;
			int start = m_Op.m_mCapStart.Get(c);
			if (start <= 0)
			{
				m_Op.m_mCapStart.Set(c, UnitsOn(cmd, c));
				continue;
			}
			int nowU = UnitsOn(cmd, c);
			if (1.0 - nowU / (start * 1.0) >= thr)
			{
				m_Op.m_aToFix.Insert(c);
				DCO_Perf.Count("op:branch_support_to_fix");
				Ev(string.Format("op_branch cmd=%1 from=%2 to=%2 reason=support_to_fix units=%3/%4", cmd.GetCommanderUID(), c.GetLogName(), nowU, start));
			}
		}
		if (m_Op.m_iMainStart <= 0)
		{
			m_Op.m_iMainStart = UnitsOn(cmd, ph.m_Main);
			return;
		}
		int mainNow = UnitsOn(cmd, ph.m_Main);
		if (1.0 - mainNow / (m_Op.m_iMainStart * 1.0) < thr || now < m_Op.m_fReserveAt)
			return;
		m_Op.m_fReserveAt = now + 60.0;
		if (now - m_Op.m_fProgressAt < 120.0)
		{
			if (mainNow <= 0 && !EmptyNeutral(cmd, ph.m_Main))
			{
				DCO_Perf.Count("op:reserve_skip_main_wiped");
				Ev(string.Format("op_reserve_skip cmd=%1 obj=%2 reason=main_wiped units=%3/%4 owner=%5 -- no own units left on the main; not feeding a single reserve group", cmd.GetCommanderUID(), ph.m_Main.GetLogName(), mainNow, m_Op.m_iMainStart, ph.m_Main.GetOwnerFaction()));
				return;
			}
			bool sent = cmd.DCO_ReinforceAssault(ph.m_Main, now, true);
			DCO_Perf.Count("op:branch_reserve_to_main");
			Ev(string.Format("op_branch cmd=%1 from=reserve to=%2 reason=reserve_to_main sent=%3 units=%4/%5", cmd.GetCommanderUID(), ph.m_Main.GetLogName(), DCO_Util.B01(sent), mainNow, m_Op.m_iMainStart));
			Ev(string.Format("op_reserve cmd=%1 commit=main obj=%2 sent=%3", cmd.GetCommanderUID(), ph.m_Main.GetLogName(), DCO_Util.B01(sent)));
		}
		else
			TryShift(cmd, ph, now, "main_losses");
	}

	protected void Intel(AICommander_BaseComponent cmd, float now)
	{
		float pct = cmd.DCO_OpIntelPct() / 100.0;
		foreach (DCO_OpObj oo : m_Op.m_aObjs)
		{
			if (!oo.m_Obj || m_Op.m_aDone.Contains(oo.m_Obj))
				continue;
			float d = Defenders(cmd, oo.m_Obj);
			float delta = Math.AbsFloat(d - oo.m_fDef0);
			if (delta < 3.0 || delta / Math.Max(oo.m_fDef0, 1.0) < pct)
			{
				m_Op.m_mIntelSince.Remove(oo.m_Obj);
				continue;
			}
			float since;
			if (!m_Op.m_mIntelSince.Find(oo.m_Obj, since))
			{
				m_Op.m_mIntelSince.Set(oo.m_Obj, now);
				continue;
			}
			int band = Band(cmd, oo.m_Obj);
			if (now - since >= 60.0 && band != oo.m_iBand0)
			{
				DCO_Perf.Count("op:replan_intel");
				Ev(string.Format("op_replan cmd=%1 reason=intel obj=%2 def0=%3 def=%4 band0=%5 band=%6", cmd.GetCommanderUID(), oo.m_Obj.GetLogName(), Math.Round(oo.m_fDef0), Math.Round(d), oo.m_iBand0, band));
				Plan(cmd, m_Op.m_Primary, now, "intel");
				return;
			}
			float logAt;
			if (!m_Op.m_mLogAt.Find(oo.m_Obj, logAt) || now >= logAt)
			{
				m_Op.m_mLogAt.Set(oo.m_Obj, now + 60.0);
				DCO_Perf.Count("op:replan_skip_hysteresis");
				Ev(string.Format("op_replan_skip cmd=%1 reason=hysteresis obj=%2 def0=%3 def=%4 held_s=%5 band_changed=%6", cmd.GetCommanderUID(), oo.m_Obj.GetLogName(), Math.Round(oo.m_fDef0), Math.Round(d), Math.Round(now - since), DCO_Util.B01(band != oo.m_iBand0)));
			}
		}
	}

	protected void ReserveExploit(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent next, float now)
	{
		vector sp;
		if (!cmd.GetStagingPos(next, sp) || sp == vector.Zero)
		{
			vector np = next.GetOwner().GetOrigin();
			vector dir = DCO_Specialist.Flat(cmd.DCO_AttackerCentroid() - np);
			sp = DCO_Util.Surface(np + dir * (next.GetRadius() + 250.0));
		}
		int sent = 0;
		string names = "";
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (sent >= 1)
				break;
			if (!g || g.IsPlayerGroup() || g.IsMortar() || g.IsArmor() || g.IsDedicatedTransport() || g.IsMedicTeam() || g.GetUnitCount() <= 0 || g.IsInContact())
				continue;
			DCO_EGroupTask t = g.GetTask();
			if (t != DCO_EGroupTask.NONE && t != DCO_EGroupTask.PATROL)
				continue;
			if (!DCO_GroupJobs.Gate(cmd, g, 129, "op_exploit"))
				continue;
			cmd.TacticMove(g, sp, now);
			sent++;
			names = g.GetOwner().GetName();
		}
		if (sent > 0)
		{
			DCO_Perf.Count("op:reserve_exploit");
			Ev(string.Format("op_reserve cmd=%1 commit=exploit grp=%2 to=%3 next=%4", cmd.GetCommanderUID(), names, DCO_Util.XZ(sp), next.GetLogName()));
		}
		int idle = 0;
		string reform = "";
		foreach (DCO_GroupUtilityComponent rg : cmd.GetOwnedGroups())
		{
			if (rg && !rg.IsPlayerGroup() && !rg.IsMortar() && (rg.GetTask() == DCO_EGroupTask.NONE || rg.GetTask() == DCO_EGroupTask.PATROL) && rg.GetUnitCount() > 0)
			{
				idle++;
				if (!reform.IsEmpty())
					reform += ",";
				reform += rg.GetOwner().GetName();
			}
		}
		if (reform.IsEmpty())
			reform = "-";
		Ev(string.Format("op_reserve_reform cmd=%1 grp=%2 n=%3", cmd.GetCommanderUID(), reform, idle));
	}

	protected void ReserveBlock(AICommander_BaseComponent cmd, float now)
	{
		if (now < m_Op.m_fReserveAt)
			return;
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (!tr)
			return;
		foreach (CMD_AICommanderObjectiveComponent d : m_Op.m_aDone)
		{
			if (!d || !d.GetOwner())
				continue;
			vector dp = d.GetOwner().GetOrigin();
			foreach (CMD_ThreatEntry t : tr.GetThreats())
			{
				if (!t || now - t.m_fLastUpdateTime > 60.0 || t.m_iEstimatedEnemyCount < 2 || vector.DistanceXZ(t.m_vBelievedPos, dp) > d.GetRadius() + 150.0)
					continue;
				m_Op.m_fReserveAt = now + 90.0;
				bool sent = cmd.DCO_ReinforceAssault(d, now, true);
				DCO_Perf.Count("op:reserve_block");
				Ev(string.Format("op_reserve cmd=%1 commit=block obj=%2 threat=%3 n=%4 sent=%5", cmd.GetCommanderUID(), d.GetLogName(), DCO_Util.XZ(t.m_vBelievedPos), t.m_iEstimatedEnemyCount, DCO_Util.B01(sent)));
				return;
			}
		}
	}

	protected void RegisterFpf(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float now)
	{
		vector op = obj.GetOwner().GetOrigin();
		vector from = vector.Zero;
		float best = float.MAX;
		foreach (DCO_OpObj oo : m_Op.m_aObjs)
		{
			if (!oo.m_Obj || oo.m_Obj == obj || m_Op.m_aDone.Contains(oo.m_Obj))
				continue;
			float d = vector.DistanceXZ(oo.m_Obj.GetOwner().GetOrigin(), op);
			if (d < best)
			{
				best = d;
				from = oo.m_Obj.GetOwner().GetOrigin();
			}
		}
		if (from == vector.Zero)
		{
			CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
			if (tr)
			{
				foreach (CMD_ThreatEntry t : tr.GetThreats())
				{
					if (!t)
						continue;
					float td = vector.DistanceXZ(t.m_vBelievedPos, op);
					if (td < best)
					{
						best = td;
						from = t.m_vBelievedPos;
					}
				}
			}
		}
		if (from == vector.Zero)
			return;
		vector dir = DCO_Specialist.Flat(from - op);
		m_Op.m_vFpfTarget = DCO_Util.Surface(op + dir * (obj.GetRadius() + 100.0));
		m_Op.m_FpfObj = obj;
		m_Op.m_fFpfUntil = now + 180.0;
		Ev(string.Format("fm_fires obj=%1 phase=fpf_registered target=%2 from=%3 until_s=180", obj.GetLogName(), DCO_Util.XZ(m_Op.m_vFpfTarget), DCO_Util.XZ(from)));
	}

	protected void Fpf(AICommander_BaseComponent cmd, float now)
	{
		if (!m_Op.m_FpfObj || now > m_Op.m_fFpfUntil)
			return;
		CMD_ArtillerySupport arty = cmd.GetArtySupport();
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		if (!arty || !arty.HasRegisteredUnits() || !tr)
			return;
		vector op = m_Op.m_FpfObj.GetOwner().GetOrigin();
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (!t || now - t.m_fLastUpdateTime > 30.0 || vector.DistanceXZ(t.m_vBelievedPos, op) > m_Op.m_FpfObj.GetRadius() + 200.0)
				continue;
			int tubes = Math.Max(1, arty.DCO_UnitCount());
			CMD_FireMissionRequest req = new CMD_FireMissionRequest(m_Op.m_vFpfTarget, SCR_EAIArtilleryAmmoType.HIGH_EXPLOSIVE, now, 4 * tubes);
			req.m_sSource = "fpf";
			bool ok = arty.RequestShellImpact(req, now, 4 * tubes);
			DCO_Perf.Count("fm_fires:fpf");
			Ev(string.Format("fm_fires obj=%1 phase=fpf target=%2 trigger=enemy_%3m rounds=%4 ok=%5", m_Op.m_FpfObj.GetLogName(), DCO_Util.XZ(m_Op.m_vFpfTarget), Math.Round(vector.DistanceXZ(t.m_vBelievedPos, op)), 4 * tubes, DCO_Util.B01(ok)));
			m_Op.m_FpfObj = null;
			return;
		}
	}

	protected void Bypass(AICommander_BaseComponent cmd, DCO_OpPhase ph, float now)
	{
		if (now < m_Op.m_fBypassAt)
			return;
		m_Op.m_fBypassAt = now + 30.0;
		CMD_ThreatResponseComponent tr = cmd.GetThreatResponseComponent();
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (!tr || !mgr)
			return;
		vector from = cmd.DCO_AttackerCentroid();
		vector to = ph.m_Main.GetOwner().GetOrigin();
		foreach (CMD_ThreatEntry t : tr.GetThreats())
		{
			if (!t || now - t.m_fLastUpdateTime > 60.0)
				continue;
			bool inObj = false;
			foreach (CMD_AICommanderObjectiveComponent o : mgr.m_aObjective)
			{
				if (o && o.GetOwner() && vector.DistanceXZ(t.m_vBelievedPos, o.GetOwner().GetOrigin()) <= o.GetRadius() + 50.0)
				{
					inObj = true;
					break;
				}
			}
			if (inObj)
				continue;
			int key = Math.Round(t.m_vBelievedPos[0] / 50.0) * 10000 + Math.Round(t.m_vBelievedPos[2] / 50.0);
			if (m_Op.m_sThreatSeen.Contains(key))
				continue;
			m_Op.m_sThreatSeen.Insert(key);
			bool onRoute = SegDist(t.m_vBelievedPos, from, to) <= 100.0;
			int n = Math.Max(1, t.m_iEstimatedEnemyCount);
			if (n <= 3 && !onRoute)
			{
				DCO_Perf.Count("op:bypass");
				Ev(string.Format("op_bypass cmd=%1 pos=%2 n=%3 action=report", cmd.GetCommanderUID(), DCO_Util.XZ(t.m_vBelievedPos), n));
				continue;
			}
			DCO_GroupUtilityComponent best;
			float bestD = float.MAX;
			foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
			{
				if (!g || g.IsPlayerGroup() || g.IsMortar() || g.IsDedicatedTransport() || g.IsMedicTeam() || g.GetUnitCount() <= 0 || !g.GetOwner())
					continue;
				DCO_EGroupTask gt = g.GetTask();
				if (gt != DCO_EGroupTask.NONE && gt != DCO_EGroupTask.PATROL)
					continue;
				if (!DCO_GroupJobs.Gate(cmd, g, 4, "op_contact_fix"))
					continue;
				float d = vector.DistanceXZ(g.GetOwner().GetOrigin(), t.m_vBelievedPos);
				if (d < bestD)
				{
					bestD = d;
					best = g;
				}
			}
			if (!best)
			{
				Ev(string.Format("op_contact_fix cmd=%1 pos=%2 n=%3 on_route=%4 grp=none", cmd.GetCommanderUID(), DCO_Util.XZ(t.m_vBelievedPos), n, DCO_Util.B01(onRoute)));
				continue;
			}
			vector dir = DCO_Specialist.Flat(best.GetOwner().GetOrigin() - t.m_vBelievedPos);
			vector pos = DCO_Util.Surface(t.m_vBelievedPos + dir * 200.0);
			best.CompleteAllWaypoints();
			cmd.TacticMove(best, pos, now);
			best.SetTask(DCO_EGroupTask.SUPPORT_BY_FIRE, "DCO_OpPlan:1182");
			DCO_Perf.Count("op:contact_fix");
			Ev(string.Format("op_contact_fix cmd=%1 pos=%2 n=%3 on_route=%4 grp=%5 at=%6", cmd.GetCommanderUID(), DCO_Util.XZ(t.m_vBelievedPos), n, DCO_Util.B01(onRoute), best.GetOwner().GetName(), DCO_Util.XZ(pos)));
		}
	}

	protected void Execute(AICommander_BaseComponent cmd, notnull array<CMD_AICommanderObjectiveComponent> list, float now)
	{
		DCO_OpPhase ph = CurPhase();
		if (!ph)
			return;
		m_Main = ph.m_Main;
		list.Clear();
		DCO_NoFireAreas.Reset(cmd);
		if (now >= m_Op.m_fConsolidateUntil)
		{
			if (cmd.DCO_MayAttack(ph.m_Main))
				list.Insert(ph.m_Main);
			foreach (CMD_AICommanderObjectiveComponent c : ph.m_aCapture)
			{
				if (c && !Done(cmd, c) && !m_Op.m_aToFix.Contains(c) && cmd.DCO_MayAttack(c))
					list.Insert(c);
			}
		}
		array<CMD_AICommanderObjectiveComponent> fixes = {};
		fixes.InsertAll(ph.m_aFix);
		foreach (CMD_AICommanderObjectiveComponent tf : m_Op.m_aToFix)
		{
			if (!fixes.Contains(tf))
				fixes.Insert(tf);
		}
		int mainOn = 0;
		int freeNow = 0;
		foreach (DCO_GroupUtilityComponent mg : cmd.GetOwnedGroups())
		{
			if (!mg || !mg.GetOwner() || mg.GetUnitCount() <= 0 || mg.IsPlayerGroup() || mg.IsMortar() || mg.IsDedicatedTransport())
				continue;
			if (mg.GetGroupObjective() == ph.m_Main)
				mainOn++;
			else if (!mg.GetGroupObjective() && mg.IsAvailableReserve() && mg.CanItHaveOrder())
				freeNow++;
		}
		int mainNeed = cmd.GetRequiredAttackGroups(ph.m_Main);
		bool mainFirst = cmd.DCO_MayAttack(ph.m_Main) && !Done(cmd, ph.m_Main) && mainOn < mainNeed && freeNow <= mainNeed - mainOn && !cmd.IsAssaultReleased(ph.m_Main);
		foreach (CMD_AICommanderObjectiveComponent f : fixes)
		{
			if (!f || Done(cmd, f) || !cmd.DCO_MayAttack(f))
				continue;
			if (DCO_Ctx.TuneForCmd(cmd, "op.occupy_empty", 0) > 0 && EmptyNeutral(cmd, f))
			{
				bool occ = cmd.DCO_ReinforceAssault(f, now, true);
				DCO_Perf.Count("op:occupy_empty_" + DCO_Util.B01(occ));
				float oat;
				if (!m_Op.m_mLogAt.Find(f, oat) || now >= oat)
				{
					m_Op.m_mLogAt.Set(f, now + 60.0);
					Ev(string.Format("op_occupy_empty cmd=%1 obj=%2 sent=%3 -- fix objective is neutral with no enemy seen; occupy instead of support by fire", cmd.GetCommanderUID(), f.GetLogName(), DCO_Util.B01(occ)));
				}
				continue;
			}
			if (mainFirst && f != ph.m_Main)
			{
				DCO_Perf.Count("op:fix_hold_main_first");
				float hat;
				if (!m_Op.m_mLogAt.Find(f, hat) || now >= hat)
				{
					m_Op.m_mLogAt.Set(f, now + 120.0);
					Ev(string.Format("op_fix_hold cmd=%1 obj=%2 main=%3 reason=main_first main_groups=%4 free=%5 need=%6 -- the main effort is below its group need; fix groups wait", cmd.GetCommanderUID(), f.GetLogName(), ph.m_Main.GetLogName(), mainOn, freeNow, mainNeed));
				}
				continue;
			}
			int want = 1;
			DCO_OpObj oo = ObjOf(m_Op, f);
			if (oo && oo.m_fDef > 15.0)
				want = 2;
			string grps = cmd.DCO_ScopeFix(f, want);
			if (grps == "none")
			{
				float at;
				if (!m_Op.m_mLogAt.Find(f, at) || now >= at)
				{
					m_Op.m_mLogAt.Set(f, now + 120.0);
					Ev(string.Format("op_flank_op cmd=%1 obj=%2 grp=none reason=no_group", cmd.GetCommanderUID(), f.GetLogName()));
				}
			}
		}
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (!g || !g.GetOwner() || g.GetUnitCount() <= 0)
				continue;
			if (g.IsMortar() || g.GetTask() == DCO_EGroupTask.SUPPORT_BY_FIRE)
				DCO_NoFireAreas.Add(cmd, g, g.GetOwner().GetOrigin());
		}
	}

	void PanelRows(notnull array<string> rows)
	{
		if (!m_Op)
			return;
		rows.Insert("H\tOperation");
		rows.Insert("V\tPrimary\t" + m_Op.m_Primary.GetLogName() + " (" + m_Op.m_sSrc + ", " + m_Op.m_sDoctrine + ")");
		foreach (int i, DCO_OpPhase ph : m_Op.m_aPhases)
		{
			string mark = "";
			if (i == m_Op.m_iCur)
				mark = " <";
			else if (m_Op.m_aDone.Contains(ph.m_Main))
				mark = " done";
			rows.Insert(string.Format("D\t%1\tMAIN %2 | capture %3 | fix %4 | %5 min%6", ph.m_iN, ph.m_Main.GetLogName(), Names(ph.m_aCapture), Names(ph.m_aFix), Math.Round(ph.m_fEta / 60.0), mark));
		}
		DCO_OpPhase cur = CurPhase();
		if (!cur)
			return;
		float now = DCO_Util.NowS();
		rows.Insert(string.Format("V\tCurrent\tphase %1, %2 s elapsed", cur.m_iN, Math.Round(now - m_Op.m_fPhaseStart)));
		string next = "MAIN " + cur.m_Main.GetLogName() + " cleared";
		if (now < m_Op.m_fConsolidateUntil)
			next = string.Format("consolidating %1 s", Math.Round(m_Op.m_fConsolidateUntil - now));
		rows.Insert("V\tNext trigger\t" + next + string.Format(" (stall in %1 s)", Math.Max(0, Math.Round(300.0 - (now - m_Op.m_fProgressAt)))));
	}

	void DbgShapes(AICommander_BaseComponent cmd, array<float> f, array<string> s, int mask, out int count)
	{
		if (!m_Op)
			return;
		vector pp = m_Op.m_Primary.GetOwner().GetOrigin();
		foreach (DCO_OpObj oo : m_Op.m_aObjs)
		{
			if (!oo.m_Obj || oo.m_Obj == m_Op.m_Primary)
				continue;
			vector op = oo.m_Obj.GetOwner().GetOrigin();
			DCO_CmdDbg.AddShape(f, s, mask, DCO_ECDLayer.OBJECTIVES, DCO_ECDShape.LINE, pp[0], pp[2], op[0], op[2], 0, DCO_ECDCol.GREY, 1, m_Op.m_sSrc, count);
		}
		vector prev = vector.Zero;
		foreach (DCO_OpPhase ph : m_Op.m_aPhases)
		{
			if (!ph.m_Main)
				continue;
			vector mp = ph.m_Main.GetOwner().GetOrigin();
			DCO_CmdDbg.AddShape(f, s, mask, DCO_ECDLayer.FOCUS, DCO_ECDShape.LETTER, mp[0], mp[2] - ph.m_Main.GetRadius() - 10.0, 0, 0, 0, DCO_ECDCol.WHITE, 0, ph.m_iN.ToString(), count);
			if (prev != vector.Zero)
				DCO_CmdDbg.AddShape(f, s, mask, DCO_ECDLayer.FOCUS, DCO_ECDShape.LINE, prev[0], prev[2], mp[0], mp[2], 0, DCO_ECDCol.PURPLE, 1, "", count);
			prev = mp;
		}
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjIsPrimaryAttribute : SCR_BaseEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAttrUtil.GetObjective(item);
		if (!obj)
			return null;
		return SCR_BaseEditorAttributeVar.CreateBool(obj.DCO_IsPrimary());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAttrUtil.GetObjective(item);
		if (!obj || !var)
			return;
		obj.DCO_SetIsPrimary(var.GetBool());
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("gm_set_obj_primary obj=%1 value=%2 player=%3", obj.GetLogName(), DCO_Util.B01(var.GetBool()), playerID));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_ObjSecondaryRadiusAttribute : SCR_BaseValueListEditorAttribute
{
	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAttrUtil.GetObjective(item);
		if (!obj)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(obj.DCO_SecondaryRadius());
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		CMD_AICommanderObjectiveComponent obj = DCO_ObjAttrUtil.GetObjective(item);
		if (!obj || !var)
			return;
		obj.DCO_SetSecondaryRadius(var.GetFloat());
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("gm_set_obj_secondary_radius obj=%1 value=%2 player=%3", obj.GetLogName(), Math.Round(var.GetFloat()), playerID));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderOpBaseAttribute : SCR_BaseValueListEditorAttribute
{
	protected float ReadOp(AICommander_BaseComponent cmd)
	{
		return 0;
	}

	protected void WriteOp(AICommander_BaseComponent cmd, float value)
	{
	}

	protected string OpName()
	{
		return "op";
	}

	override SCR_BaseEditorAttributeVar ReadVariable(Managed item, SCR_AttributesManagerEditorComponent manager)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd)
			return null;
		return SCR_BaseEditorAttributeVar.CreateFloat(ReadOp(cmd));
	}

	override void WriteVariable(Managed item, SCR_BaseEditorAttributeVar var, SCR_AttributesManagerEditorComponent manager, int playerID)
	{
		AICommander_BaseComponent cmd = DCO_CommanderBaseAttribute.GetCommander(item);
		if (!cmd || !var)
			return;
		WriteOp(cmd, var.GetFloat());
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", string.Format("gm_set_%1 cmd=%2 value=%3 player=%4", OpName(), cmd.GetCommanderUID(), var.GetFloat().ToString(-1, 0), playerID));
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderOpStallAttribute : DCO_CommanderOpBaseAttribute
{
	override protected float ReadOp(AICommander_BaseComponent cmd) { return cmd.DCO_OpStallS(); }
	override protected void WriteOp(AICommander_BaseComponent cmd, float value) { cmd.DCO_SetOpStallS(value); }
	override protected string OpName() { return "op_stall"; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderOpIntelAttribute : DCO_CommanderOpBaseAttribute
{
	override protected float ReadOp(AICommander_BaseComponent cmd) { return cmd.DCO_OpIntelPct(); }
	override protected void WriteOp(AICommander_BaseComponent cmd, float value) { cmd.DCO_SetOpIntelPct(value); }
	override protected string OpName() { return "op_intel"; }
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_CommanderOpLossAttribute : DCO_CommanderOpBaseAttribute
{
	override protected float ReadOp(AICommander_BaseComponent cmd) { return cmd.DCO_OpLossPct(); }
	override protected void WriteOp(AICommander_BaseComponent cmd, float value) { cmd.DCO_SetOpLossPct(value); }
	override protected string OpName() { return "op_losses"; }
}
