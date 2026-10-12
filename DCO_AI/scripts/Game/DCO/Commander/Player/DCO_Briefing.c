enum DCO_EBriefKind
{
	OPORD,
	FRAGO,
	PHASE,
	WITHDRAW,
	ABORT,
	WARNING,
	END
}

enum DCO_EBriefPt
{
	OBJ = 1,
	THREAT = 2,
	TRIGGER = 3,
	TRP = 4,
	FRIEND = 5,
	RESERVE = 6,
	WITHDRAW = 7,
	RP = 8,
	AXIS = 9,
	SBF = 10,
	ASSAULT = 11,
	BLOCK = 12,
	OWN = 13,
	SECTOR = 14,
	ROUTE = 15
}

enum DCO_EBriefFlag
{
	LIFT = 1,
	FIRE = 2,
	HHOUR = 4,
	FINAL = 8
}

class DCO_BriefMsg
{
	int m_iKind;
	int m_iFlags;
	string m_sType;
	string m_sObj;
	string m_sLogObj;
	string m_sTask;
	string m_sHead;
	string m_sLines;
	int m_iLines;
	ref array<float> m_aPts = {};
	float m_fHHour = -1;
	vector m_vObj;
}

class DCO_Briefing
{
	protected static ref map<int, float> s_mSent;
	protected static ref map<int, ref DCO_BriefMsg> s_mPending;
	protected static ref map<string, string> s_mCode;
	protected static ref map<string, float> s_mHHour;
	protected static ref map<string, int> s_mFlags;
	protected static ref map<string, string> s_mLastHead;
	protected static ref map<string, ref array<int>> s_mActive;
	protected static bool s_bFlushQueued;

	static bool Enabled()
	{
		if (!Replication.IsServer())
			return false;
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		return !cfg || cfg.GetPlayerBriefings();
	}

	protected static void Init()
	{
		if (s_mSent)
			return;
		s_mSent = new map<int, float>();
		s_mPending = new map<int, ref DCO_BriefMsg>();
		s_mCode = new map<string, string>();
		s_mHHour = new map<string, float>();
		s_mFlags = new map<string, int>();
		s_mLastHead = new map<string, string>();
		s_mActive = new map<string, ref array<int>>();
	}

	static string KindName(int kind)
	{
		switch (kind)
		{
			case DCO_EBriefKind.OPORD: return "opord";
			case DCO_EBriefKind.FRAGO: return "frago";
			case DCO_EBriefKind.PHASE: return "phase";
			case DCO_EBriefKind.WITHDRAW: return "withdraw";
			case DCO_EBriefKind.ABORT: return "abort";
			case DCO_EBriefKind.WARNING: return "threat_warning";
		}
		return "end";
	}

	protected static int Rank(int kind)
	{
		switch (kind)
		{
			case DCO_EBriefKind.OPORD: return 3;
			case DCO_EBriefKind.FRAGO: return 2;
			case DCO_EBriefKind.PHASE: return 1;
		}
		return 0;
	}

	protected static string T(string key, array<string> params = null)
	{
		return DCO_Radio.Resolve(key, 0, params);
	}

	protected static string Add(string lines, string labelKey, string text)
	{
		string row = T(labelKey) + "|" + text;
		if (lines.IsEmpty())
			return row;
		return lines + "\n" + row;
	}

	protected static string Code(string objKey)
	{
		Init();
		string code;
		if (s_mCode.Find(objKey, code))
			return code;
		code = DCO_Radio.Text("brief_codeword", null, null);
		s_mCode.Set(objKey, code);
		return code;
	}

	protected static string Low(string s)
	{
		string r = s;
		r.ToLower();
		return r;
	}

	protected static string DistDir(vector from, vector to)
	{
		return DCO_Radio.N(vector.DistanceXZ(from, to)) + " m " + DCO_PlayerComms.Bearing(from, to);
	}

	protected static void Pt(array<float> pts, int type, bool own, vector a, vector b)
	{
		pts.Insert(type);
		if (own)
			pts.Insert(1);
		else
			pts.Insert(0);
		pts.Insert(a[0]);
		pts.Insert(a[2]);
		pts.Insert(b[0]);
		pts.Insert(b[2]);
	}

	protected static void Recipients(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, notnull array<int> tasked, notnull array<int> informed, notnull array<int> warned)
	{
		cmd.GetPlayerTasking().GetGroupsOnObjective(obj, tasked);
		vector objPos = obj.GetOwner().GetOrigin();
		float radio = 800;
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		if (cfg)
			radio = cfg.GetRadioRadius();
		float warnR = obj.GetRadius() + 600.0;
		array<int> near = {};
		DCO_PlayerAwareness.CollectPlayerGroupsNear(cmd.GetCommanderFactionKey(), objPos, Math.Max(radio, warnR), near);
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (!groups)
			return;
		foreach (int id : near)
		{
			if (tasked.Contains(id))
				continue;
			SCR_AIGroup grp = groups.FindGroup(id);
			if (!grp)
				continue;
			float d = vector.DistanceXZ(DCO_PlayerTasking.LeaderPos(grp), objPos);
			bool ours = !grp.DCO_IsDecliningOrders() && grp.DCO_GetOrdersCommander() == cmd;
			if (ours && d <= radio)
				informed.Insert(id);
			else if (d <= warnR)
				warned.Insert(id);
		}
	}

	static void HHour(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, float sec)
	{
		if (!Enabled() || !cmd || !obj)
			return;
		Init();
		s_mHHour.Set(obj.GetLogName(), DCO_Util.NowS() + sec);
		DCO_TacticPlan p = cmd.GetTactics().GetPlan(obj);
		if (p)
			Attack(cmd, p, DCO_EBriefKind.PHASE, "brief_phase_hhour", DCO_Radio.P("obj", obj.GetDisplayName(), "sec", DCO_Radio.N(sec)));
	}

	static void AttackFlag(AICommander_BaseComponent cmd, DCO_TacticPlan p, int flag, string headKey)
	{
		if (!Enabled() || !p || !p.m_Obj)
			return;
		Init();
		string objKey = p.m_Obj.GetLogName();
		int flags = 0;
		if (!s_mFlags.Find(objKey, flags))
			flags = 0;
		s_mFlags.Set(objKey, flags | flag);
		Attack(cmd, p, DCO_EBriefKind.PHASE, headKey, DCO_Radio.P("obj", p.m_Obj.GetDisplayName()));
	}

	protected static string AttackType(int tactic)
	{
		switch (tactic)
		{
			case DCO_ETactic.RAID:
			case DCO_ETactic.RECON_IN_FORCE:
			case DCO_ETactic.ATTACK_BY_FIRE:
			case DCO_ETactic.SIEGE:
			case DCO_ETactic.BYPASS:
				return "brief_type_attack";
			case DCO_ETactic.DELAY:
				return "brief_type_delay";
		}
		return "brief_type_seize";
	}

	protected static string RoleKey(int role)
	{
		string k = DCO_CommanderTactics.RoleName(role);
		k.ToLower();
		return k;
	}

	static void Attack(AICommander_BaseComponent cmd, DCO_TacticPlan p, int kind, string headKey = "", array<string> headParams = null)
	{
		if (!Enabled() || !cmd || !p || !p.m_Obj)
			return;
		Init();
		CMD_AICommanderObjectiveComponent obj = p.m_Obj;
		string objKey = obj.GetLogName();
		string objName = obj.GetDisplayName();
		vector objPos = obj.GetOwner().GetOrigin();
		array<int> tasked = {};
		array<int> informed = {};
		array<int> warned = {};
		Recipients(cmd, obj, tasked, informed, warned);
		foreach (int rid, int rr : p.m_mPlayerRoles)
		{
			if (!tasked.Contains(rid))
				tasked.Insert(rid);
			informed.RemoveItem(rid);
		}
		if (tasked.IsEmpty() && informed.IsEmpty() && warned.IsEmpty())
			return;

		vector staging = cmd.TacticStagingPos(obj);
		vector rally = staging;
		string tacName = T("brief_tac_" + Low(DCO_CommanderTactics.Name(p.m_eTactic)));
		string typeKey = AttackType(p.m_eTactic);
		if (kind == DCO_EBriefKind.ABORT)
			typeKey = "brief_type_withdraw";

		int nAssault = 0;
		int nFix = 0;
		int nBlock = 0;
		foreach (int ri, int role : p.m_aRoles)
		{
			if (role == DCO_CommanderTactics.ROLE_FIX)
				nFix++;
			else if (role == DCO_CommanderTactics.ROLE_BLOCK)
				nBlock++;
			else
				nAssault++;
		}
		foreach (int pid2, int pr : p.m_mPlayerRoles)
		{
			if (pr == DCO_CommanderTactics.ROLE_FIX)
				nFix++;
			else if (pr == DCO_CommanderTactics.ROLE_BLOCK)
				nBlock++;
			else
				nAssault++;
		}

		array<float> basePts = {};
		Pt(basePts, DCO_EBriefPt.OBJ, false, objPos, Vector(obj.GetRadius(), 0, 0));
		Pt(basePts, DCO_EBriefPt.AXIS, false, staging, objPos);
		Pt(basePts, DCO_EBriefPt.RP, false, rally, rally);
		foreach (int gi, DCO_GroupUtilityComponent g : p.m_aGroups)
		{
			if (!g || !g.GetOwner())
				continue;
			int grole = p.m_aRoles[gi];
			vector gpos = p.m_aGoals[gi];
			if (grole == DCO_CommanderTactics.ROLE_FIX)
				Pt(basePts, DCO_EBriefPt.SBF, false, gpos, objPos);
			else if (grole == DCO_CommanderTactics.ROLE_BLOCK)
				Pt(basePts, DCO_EBriefPt.BLOCK, false, gpos, objPos);
			else
				Pt(basePts, DCO_EBriefPt.ASSAULT, false, g.GetOwner().GetOrigin(), gpos);
		}

		float hh = -1;
		float hhAt;
		if (s_mHHour.Find(objKey, hhAt) && hhAt > DCO_Util.NowS())
			hh = hhAt - DCO_Util.NowS();
		int flags = 0;
		if (!s_mFlags.Find(objKey, flags))
			flags = 0;
		if (hh > 0)
			flags = flags | DCO_EBriefFlag.HHOUR;

		string head;
		if (!headKey.IsEmpty())
		{
			array<string> hp = headParams;
			if (!hp)
				hp = DCO_Radio.P("obj", objName, "tactic", tacName, "grid", DCO_PlayerComms.Grid(rally));
			head = T(headKey, hp);
		}
		if (kind == DCO_EBriefKind.PHASE && !Changed(objKey, head))
			return;

		string hhText = T("brief_hhour_staging");
		if (hh > 0)
			hhText = T("brief_hhour_in", DCO_Radio.P("sec", DCO_Radio.N(hh)));
		string common = Add("", "brief_lbl_atask", T("brief_atk_task", DCO_Radio.P("tactic", tacName, "obj", objName, "dir", DCO_PlayerComms.Bearing(objPos, staging), "hhour", hhText)));

		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		array<int> all = {};
		all.Copy(tasked);
		foreach (int inf : informed)
			all.Insert(inf);
		foreach (int id : all)
		{
			SCR_AIGroup grp = groups.FindGroup(id);
			if (!grp)
				continue;
			int myRole = -1;
			if (!p.m_mPlayerRoles.Find(id, myRole))
				myRole = -1;
			vector goal = objPos;
			if (!p.m_mPlayerGoals.Find(id, goal))
				goal = objPos;
			string roleText = "";
			string task = "";
			if (myRole >= 0)
			{
				roleText = T("brief_role_" + RoleKey(myRole), DCO_Radio.P("obj", objName, "pos", DistDir(objPos, goal), "dir", DCO_PlayerComms.Bearing(objPos, goal)));
				task = T("brief_task_" + RoleKey(myRole), DCO_Radio.P("obj", objName, "dir", DCO_PlayerComms.Bearing(objPos, goal)));
			}
			else if (tasked.Contains(id))
			{
				roleText = T("brief_role_tasked", DCO_Radio.P("obj", objName));
				task = T("brief_task_tasked", DCO_Radio.P("obj", objName));
			}
			else
			{
				roleText = T("brief_role_none", DCO_Radio.P("obj", objName));
				task = T("brief_task_none", DCO_Radio.P("obj", objName));
			}
			string lines = Add(common, "brief_lbl_role", roleText);
			lines = Add(lines, "brief_lbl_friendly", T("brief_atk_friendly", DCO_Radio.P("assault", nAssault.ToString(), "fix", nFix.ToString(), "block", nBlock.ToString())));
			lines = Add(lines, "brief_lbl_signals", T("brief_atk_signals"));
			lines = Add(lines, "brief_lbl_after", T("brief_atk_after", DCO_Radio.P("obj", objName, "grid", DCO_PlayerComms.Grid(rally))));

			DCO_BriefMsg m = new DCO_BriefMsg();
			m.m_iKind = kind;
			m.m_iFlags = flags;
			m.m_sType = T(typeKey);
			m.m_sObj = objName;
			m.m_sLogObj = objKey;
			m.m_sTask = task;
			m.m_sHead = head;
			m.m_sLines = lines;
			m.m_iLines = 5;
			m.m_fHHour = hh;
			m.m_vObj = objPos;
			m.m_aPts.Copy(basePts);
			foreach (int oid, int orole : p.m_mPlayerRoles)
			{
				vector og = objPos;
				if (!p.m_mPlayerGoals.Find(oid, og))
					og = objPos;
				int t = DCO_EBriefPt.ASSAULT;
				if (orole == DCO_CommanderTactics.ROLE_FIX)
					t = DCO_EBriefPt.SBF;
				else if (orole == DCO_CommanderTactics.ROLE_BLOCK)
					t = DCO_EBriefPt.BLOCK;
				SCR_AIGroup og2 = groups.FindGroup(oid);
				vector from = og;
				if (og2)
					from = DCO_PlayerTasking.LeaderPos(og2);
				if (t == DCO_EBriefPt.ASSAULT)
					Pt(m.m_aPts, t, oid == id, from, og);
				else
					Pt(m.m_aPts, t, oid == id, og, objPos);
			}
			vector lp = DCO_PlayerTasking.LeaderPos(grp);
			Pt(m.m_aPts, DCO_EBriefPt.ROUTE, true, lp, goal);
			Pt(m.m_aPts, DCO_EBriefPt.OWN, true, lp, lp);
			Deliver(id, m);
		}
		Track(objKey, all);
		if (kind == DCO_EBriefKind.OPORD)
			Warn(cmd, obj, warned);
	}

	static void Defense(AICommander_BaseComponent cmd, DCO_DefensePlan p, int kind, string headKey = "", array<string> headParams = null)
	{
		if (!Enabled() || !cmd || !p || !p.m_Obj)
			return;
		Init();
		CMD_AICommanderObjectiveComponent obj = p.m_Obj;
		string objKey = obj.GetLogName();
		string objName = obj.GetDisplayName();
		vector objPos = obj.GetOwner().GetOrigin();
		array<int> tasked = {};
		array<int> informed = {};
		array<int> warned = {};
		Recipients(cmd, obj, tasked, informed, warned);
		if (tasked.IsEmpty() && informed.IsEmpty() && warned.IsEmpty())
			return;

		float R = obj.GetRadius();
		vector axis = p.m_vAxis;
		if (axis == vector.Zero)
			axis = DCO_Specialist.Flat(p.m_vThreat - objPos);
		axis = DCO_Specialist.Flat(axis);
		vector side = Vector(-axis[2], 0, axis[0]);
		vector threatPos = p.m_vThreat;
		if (threatPos == vector.Zero)
			threatPos = objPos + axis * (R + 400.0);
		vector rp = objPos - axis * (R + 250.0);
		string threatDir = DCO_PlayerComms.Bearing(objPos, threatPos);
		string code = Code(objKey);

		string typeKey = "brief_type_hold";
		if (p.m_eTactic == DCO_EDefTactic.DELAY)
			typeKey = "brief_type_delay";
		if (kind == DCO_EBriefKind.WITHDRAW || kind == DCO_EBriefKind.ABORT)
			typeKey = "brief_type_withdraw";

		string tacName = T("brief_deftac_" + Low(DCO_DefenseTactics.Name(p.m_eTactic)));
		string enemyKey = "brief_def_enemy";
		if (p.m_bArmor || p.m_bIDF)
			enemyKey = "brief_def_enemy_heavy";
		string lines = Add("", "brief_lbl_enemy", T(enemyKey, DCO_Radio.P("dir", threatDir, "dist", DCO_Radio.N(vector.DistanceXZ(objPos, threatPos)), "axes", p.m_iAxes.ToString())));
		string common2;
		int alive = 0;
		foreach (DCO_GroupUtilityComponent dg : p.m_aGroups)
		{
			if (dg && dg.GetOwner())
				alive++;
		}
		string friendKey = "brief_def_friendly";
		if (p.m_Reserve)
			friendKey = "brief_def_friendly_res";
		common2 = Add(common2, "brief_lbl_friendly", T(friendKey, DCO_Radio.P("groups", alive.ToString(), "dir", DCO_PlayerComms.Bearing(objPos, p.m_vReservePos))));
		string firesKey = "brief_def_fires_open";
		if (p.m_fTrigger > 0)
			firesKey = "brief_def_fires";
		if (p.m_bEA)
			firesKey = "brief_def_fires_ea";
		common2 = Add(common2, "brief_lbl_fires", T(firesKey, DCO_Radio.P("trigger", DCO_Radio.N(p.m_fTrigger), "dir", threatDir)));
		common2 = Add(common2, "brief_lbl_withdraw", T("brief_def_withdraw", DCO_Radio.P("grid", DCO_PlayerComms.Grid(rp), "pos", DistDir(objPos, rp), "code", code)));

		array<float> basePts = {};
		Pt(basePts, DCO_EBriefPt.OBJ, false, objPos, Vector(R, 0, 0));
		Pt(basePts, DCO_EBriefPt.THREAT, false, threatPos, objPos + axis * R);
		if (p.m_fTrigger > 0)
		{
			vector tc = objPos + axis * p.m_fTrigger;
			Pt(basePts, DCO_EBriefPt.TRIGGER, false, tc - side * 150.0, tc + side * 150.0);
		}
		vector trp = objPos + axis * (Math.Max(p.m_fTrigger, R) + 100.0);
		if (p.m_bEA)
			trp = p.m_vEACenter;
		Pt(basePts, DCO_EBriefPt.TRP, false, trp, trp);
		foreach (DCO_GroupUtilityComponent fg : p.m_aGroups)
		{
			if (fg && fg.GetOwner() && fg != p.m_Reserve)
				Pt(basePts, DCO_EBriefPt.FRIEND, false, fg.GetOwner().GetOrigin(), fg.GetOwner().GetOrigin());
		}
		if (p.m_Reserve)
			Pt(basePts, DCO_EBriefPt.RESERVE, false, p.m_vReservePos, p.m_vReservePos);
		Pt(basePts, DCO_EBriefPt.WITHDRAW, false, objPos, rp);
		Pt(basePts, DCO_EBriefPt.RP, false, rp, rp);

		string head;
		if (!headKey.IsEmpty())
		{
			array<string> hp = headParams;
			if (!hp)
				hp = DCO_Radio.P("obj", objName, "grid", DCO_PlayerComms.Grid(rp), "code", code, "dir", threatDir, "tactic", tacName);
			head = T(headKey, hp);
		}
		if (kind == DCO_EBriefKind.PHASE && !Changed(objKey, head))
			return;
		int flags = 0;
		if (!s_mFlags.Find(objKey, flags))
			flags = 0;
		if (p.m_bFired || p.m_bEAFire)
			flags = flags | DCO_EBriefFlag.FIRE;

		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		array<int> all = {};
		all.Copy(tasked);
		foreach (int inf : informed)
			all.Insert(inf);
		foreach (int id : all)
		{
			SCR_AIGroup grp = groups.FindGroup(id);
			if (!grp)
				continue;
			vector lp = DCO_PlayerTasking.LeaderPos(grp);
			string sector = DCO_PlayerComms.Bearing(objPos, lp);
			if (vector.DistanceXZ(objPos, lp) < R * 0.3)
				sector = threatDir;
			string taskKey = "brief_def_task";
			if (!tasked.Contains(id))
				taskKey = "brief_def_task_near";
			string myTask = T(taskKey, DCO_Radio.P("tactic", tacName, "obj", objName, "sector", sector, "dir", threatDir));
			DCO_BriefMsg m = new DCO_BriefMsg();
			m.m_iKind = kind;
			m.m_iFlags = flags;
			m.m_sType = T(typeKey);
			m.m_sObj = objName;
			m.m_sLogObj = objKey;
			m.m_sTask = T("brief_task_hold", DCO_Radio.P("obj", objName, "sector", sector));
			if (kind == DCO_EBriefKind.WITHDRAW)
				m.m_sTask = T("brief_task_withdraw", DCO_Radio.P("grid", DCO_PlayerComms.Grid(rp)));
			m.m_sHead = head;
			m.m_sLines = Add(lines, "brief_lbl_task", myTask) + "\n" + common2;
			m.m_iLines = 5;
			m.m_vObj = objPos;
			m.m_aPts.Copy(basePts);
			vector look = DCO_Specialist.Flat(threatPos - lp);
			vector lside = Vector(-look[2], 0, look[0]);
			Pt(m.m_aPts, DCO_EBriefPt.OWN, true, lp, lp);
			Pt(m.m_aPts, DCO_EBriefPt.SECTOR, true, lp, lp + (look - lside) * 150.0);
			Pt(m.m_aPts, DCO_EBriefPt.SECTOR, true, lp, lp + (look + lside) * 150.0);
			Deliver(id, m);
		}
		Track(objKey, all);
		if (kind == DCO_EBriefKind.OPORD)
			Warn(cmd, obj, warned);
	}

	protected static bool Changed(string objKey, string head)
	{
		string last;
		if (s_mLastHead.Find(objKey, last) && last == head)
			return false;
		s_mLastHead.Set(objKey, head);
		return true;
	}

	protected static void Track(string objKey, array<int> ids)
	{
		array<int> known = s_mActive.Get(objKey);
		if (!known)
		{
			known = {};
			s_mActive.Set(objKey, known);
		}
		foreach (int id : ids)
		{
			if (!known.Contains(id))
				known.Insert(id);
		}
	}

	protected static void Warn(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, array<int> warned)
	{
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (!groups)
			return;
		vector objPos = obj.GetOwner().GetOrigin();
		foreach (int id : warned)
		{
			SCR_AIGroup grp = groups.FindGroup(id);
			if (!grp)
				continue;
			vector lp = DCO_PlayerTasking.LeaderPos(grp);
			DCO_BriefMsg m = new DCO_BriefMsg();
			m.m_iKind = DCO_EBriefKind.WARNING;
			m.m_sType = T("brief_hdr_warning");
			m.m_sObj = obj.GetDisplayName();
			m.m_sLogObj = obj.GetLogName();
			m.m_sTask = T("brief_warn", DCO_Radio.P("obj", obj.GetDisplayName(), "pos", DistDir(lp, objPos)));
			m.m_sHead = m.m_sTask;
			m.m_vObj = objPos;
			Deliver(id, m);
		}
	}

	static void Close(CMD_AICommanderObjectiveComponent obj, int kind, string headKey, array<string> params)
	{
		if (!Enabled() || !obj)
			return;
		Init();
		string objKey = obj.GetLogName();
		array<int> ids = s_mActive.Get(objKey);
		s_mHHour.Remove(objKey);
		s_mFlags.Remove(objKey);
		s_mLastHead.Remove(objKey);
		s_mCode.Remove(objKey);
		if (!ids)
			return;
		s_mActive.Remove(objKey);
		string head = T(headKey, params);
		string typeKey = "brief_type_withdraw";
		if (kind == DCO_EBriefKind.END)
			typeKey = "brief_hdr_orders";
		foreach (int id : ids)
		{
			DCO_BriefMsg pend = s_mPending.Get(id);
			if (pend && pend.m_sLogObj == objKey)
				s_mPending.Remove(id);
			DCO_BriefMsg m = new DCO_BriefMsg();
			m.m_iKind = kind;
			m.m_iFlags = DCO_EBriefFlag.FINAL;
			m.m_sType = T(typeKey);
			m.m_sObj = obj.GetDisplayName();
			m.m_sLogObj = objKey;
			m.m_sTask = head;
			m.m_sHead = head;
			m.m_iLines = 1;
			m.m_vObj = obj.GetOwner().GetOrigin();
			Deliver(id, m);
		}
	}

	static void DefenseEnd(DCO_DefensePlan p, string result)
	{
		if (!Enabled() || !p || !p.m_Obj)
			return;
		Init();
		CMD_AICommanderObjectiveComponent obj = p.m_Obj;
		vector objPos = obj.GetOwner().GetOrigin();
		vector axis = DCO_Specialist.Flat(p.m_vAxis);
		vector rp = objPos - axis * (obj.GetRadius() + 250.0);
		array<string> params = DCO_Radio.P("obj", obj.GetDisplayName(), "grid", DCO_PlayerComms.Grid(rp), "code", Code(obj.GetLogName()));
		if (result == "withdrawn" || result == "lost")
			Close(obj, DCO_EBriefKind.WITHDRAW, "brief_withdraw", params);
		else if (result == "delayed")
			Close(obj, DCO_EBriefKind.END, "brief_phase_delayed", params);
		else
			Close(obj, DCO_EBriefKind.END, "brief_phase_held", params);
	}

	protected static void Deliver(int groupID, DCO_BriefMsg m)
	{
		Init();
		float now = DCO_Util.NowS();
		bool urgent = m.m_iKind == DCO_EBriefKind.WITHDRAW || m.m_iKind == DCO_EBriefKind.ABORT || m.m_iKind == DCO_EBriefKind.END;
		float last = -1000;
		if (!s_mSent.Find(groupID, last))
			last = -1000;
		bool throttled = !urgent && now - last < 20.0;
		DCO_Log.Write(DCO_ELogChannel.PLAYER, DCO_ELogLevel.INFO, "-", string.Format("brief_send obj=%1 kind=%2 to=%3 lines=%4 throttled=%5 flags=%6 hhour_s=%7", m.m_sLogObj, KindName(m.m_iKind), groupID, m.m_iLines, throttled, m.m_iFlags, Math.Round(m.m_fHHour)));
		if (throttled)
		{
			DCO_BriefMsg old = s_mPending.Get(groupID);
			if (old && Rank(old.m_iKind) > Rank(m.m_iKind))
			{
				if (m.m_iKind == DCO_EBriefKind.PHASE)
					return;
				m.m_iKind = old.m_iKind;
			}
			s_mPending.Set(groupID, m);
			DCO_Perf.Count("brief:throttled");
			if (!s_bFlushQueued)
			{
				s_bFlushQueued = true;
				GetGame().GetCallqueue().CallLater(Flush, Math.Max(1000, (20.0 - (now - last)) * 1000.0), false);
			}
			return;
		}
		s_mPending.Remove(groupID);
		s_mSent.Set(groupID, now);
		Push(groupID, m);
	}

	protected static void Flush()
	{
		s_bFlushQueued = false;
		if (!s_mPending || s_mPending.IsEmpty())
			return;
		float now = DCO_Util.NowS();
		float wait = 20.0;
		array<int> ready = {};
		foreach (int gid, DCO_BriefMsg pm : s_mPending)
		{
			float last = -1000;
			if (!s_mSent.Find(gid, last))
				last = -1000;
			if (now - last >= 20.0)
				ready.Insert(gid);
			else
				wait = Math.Min(wait, 20.0 - (now - last));
		}
		foreach (int rid : ready)
		{
			DCO_BriefMsg rm = s_mPending.Get(rid);
			s_mPending.Remove(rid);
			s_mSent.Set(rid, now);
			if (rm)
				Push(rid, rm);
		}
		if (!s_mPending.IsEmpty())
		{
			s_bFlushQueued = true;
			GetGame().GetCallqueue().CallLater(Flush, Math.Max(1000, wait * 1000.0), false);
		}
	}

	protected static void Push(int groupID, DCO_BriefMsg m)
	{
		SCR_GroupsManagerComponent groups = SCR_GroupsManagerComponent.GetInstance();
		if (!groups)
			return;
		SCR_AIGroup grp = groups.FindGroup(groupID);
		if (!grp)
			return;
		array<int> players = grp.GetPlayerIDs();
		if (!players)
			return;
		DCO_Perf.Count("brief:" + KindName(m.m_iKind));
		foreach (int pid : players)
		{
			SCR_PlayerControllerGroupComponent comp = SCR_PlayerControllerGroupComponent.GetPlayerControllerComponent(pid);
			if (comp)
				comp.DCO_SendBrief(m.m_iKind, m.m_iFlags, m.m_sType, m.m_sObj, m.m_sTask, m.m_sHead, m.m_sLines, m.m_aPts, Vector(m.m_vObj[0], m.m_fHHour, m.m_vObj[2]));
		}
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_PlayerBriefingsAttribute : DCO_GlobalBoolAttribute
{
	override protected bool Get(DCO_GlobalAIComponent cfg) { return cfg.GetPlayerBriefings(); }
	override protected void Set(DCO_GlobalAIComponent cfg, bool value)
	{
		cfg.SetPlayerBriefings(value);
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "gm_set_player_briefings on=" + value.ToString());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_PlayerBriefingCapAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetPlayerBriefingCap(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value)
	{
		cfg.SetPlayerBriefingCap(value);
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "gm_set_player_briefing_cap cap=" + cfg.GetPlayerBriefingCap().ToString());
	}
}

[BaseContainerProps(), SCR_BaseEditorAttributeCustomTitle()]
class DCO_BriefingWorldMarkersAttribute : DCO_GlobalFloatAttribute
{
	override protected float Get(DCO_GlobalAIComponent cfg) { return cfg.GetBriefingWorldMarkers(); }
	override protected void Set(DCO_GlobalAIComponent cfg, float value)
	{
		cfg.SetBriefingWorldMarkers(value);
		DCO_Log.Write(DCO_ELogChannel.GM, DCO_ELogLevel.INFO, "-", "gm_set_briefing_world_markers mode=" + cfg.GetBriefingWorldMarkers().ToString());
	}
}
