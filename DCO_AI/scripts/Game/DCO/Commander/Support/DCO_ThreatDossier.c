class DCO_ThreatResp
{
	int m_iAction;
	float m_fAt;
	ref array<DCO_GroupUtilityComponent> m_aGroups = {};
	ref array<int> m_aStart = {};
}

class DCO_ThreatDossier
{
	int m_iId;
	string m_sClass;
	vector m_vPos;
	float m_fFirstAt;
	float m_fLastAt;
	int m_iMaxEnemies;
	int m_iGroupsKilled;
	int m_iEsc;
	float m_fLossAt = -100000.0;
	int m_iPrefer = -1;
	string m_sMeans;
	bool m_bHPT;
	bool m_bDecoy;
	float m_fDecoyAt;
	float m_fDrawAt = -100000.0;
	float m_fRatio = 1.0;
	bool m_bCanMatch = true;
	bool m_bProtected;
	int m_iIneff;
	int m_iClaimMission = -1;
	float m_fClaimAt = -1;
	ref map<int, float> m_mBan = new map<int, float>();
	ref DCO_ThreatResp m_Resp;
}

class DCO_ThreatBook
{
	protected static int s_iNext;
	protected AICommander_BaseComponent m_Cmd;
	protected ref array<ref DCO_ThreatDossier> m_aD = {};
	protected ref map<string, int> m_mSector = new map<string, int>();
	protected ref map<string, float> m_mSectorAt = new map<string, float>();
	protected ref map<int, string> m_mCapLog = new map<int, string>();

	void DCO_ThreatBook(AICommander_BaseComponent cmd)
	{
		m_Cmd = cmd;
	}

	protected void Log(string line)
	{
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, "-", line);
	}

	static int Rank(string cls)
	{
		if (cls == "armor")
			return 4;
		if (cls == "veh" || cls == "arty")
			return 3;
		if (cls == "inf_mgat")
			return 2;
		if (cls == "sniper")
			return 1;
		return 0;
	}

	static string ClassOf(CMD_ThreatAssessment a, CMD_ThreatCluster c)
	{
		if (a.m_eType == CMD_EThreatType.ARMOR)
			return "armor";
		bool arty;
		bool veh;
		foreach (CMD_ThreatEntry e : c.m_aMembers)
		{
			if (!e)
				continue;
			arty = arty || e.m_bArtillery;
			veh = veh || e.m_iVehicle > 0;
		}
		if (arty)
			return "arty";
		if (veh || a.m_eType == CMD_EThreatType.MOTORIZED)
			return "veh";
		if (a.m_bSniper && a.m_iEnemies <= 3)
			return "sniper";
		if (a.m_bMG || a.m_bAT)
			return "inf_mgat";
		return "inf";
	}

	DCO_ThreatDossier Get(int id)
	{
		foreach (DCO_ThreatDossier d : m_aD)
		{
			if (d.m_iId == id)
				return d;
		}
		return null;
	}

	protected DCO_ThreatDossier Find(vector pos, float now, int missionId)
	{
		DCO_ThreatDossier best;
		float bestD = 250.0;
		foreach (DCO_ThreatDossier d : m_aD)
		{
			if (now - d.m_fLastAt > 900.0)
				continue;
			if (d.m_fClaimAt == now && d.m_iClaimMission != missionId)
				continue;
			float dist = vector.DistanceXZ(d.m_vPos, pos);
			if (dist <= bestD)
			{
				bestD = dist;
				best = d;
			}
		}
		return best;
	}

	protected CMD_AICommanderObjectiveComponent OwnObjNear(vector pos)
	{
		FactionKey fk = m_Cmd.GetCommanderFactionKey();
		CMD_AICommanderObjectiveComponent best;
		float bestD = 1500.0;
		foreach (CMD_AICommanderObjectiveComponent obj : m_Cmd.DCO_AllObjectives())
		{
			if (!obj || !obj.GetOwner() || obj.GetOwnerFaction() != fk)
				continue;
			float d = vector.DistanceXZ(obj.GetOwner().GetOrigin(), pos);
			if (d < bestD)
			{
				bestD = d;
				best = obj;
			}
		}
		return best;
	}

	protected void NoteSector(DCO_ThreatDossier d, CMD_ThreatAssessment a, float now)
	{
		CMD_AICommanderObjectiveComponent obj = OwnObjNear(d.m_vPos);
		if (!obj)
			return;
		vector objPos = obj.GetOwner().GetOrigin();
		vector dir = d.m_vPos - objPos;
		dir[1] = 0;
		if (dir.LengthSq() < 100.0)
			return;
		int s = Math.Round(Math.Repeat(dir.ToYaw(), 360.0) / 45.0);
		s = s % 8;
		string key = obj.GetLogName() + "|" + s.ToString();
		float lastAt;
		if (m_mSectorAt.Find(key, lastAt) && now - lastAt > 1200.0)
			m_mSector.Set(key, 0);
		m_mSectorAt.Set(key, now);
		int n = m_mSector.Get(key) + 1;
		if (n > 3)
			n = 1;
		m_mSector.Set(key, n);
		DCO_CommanderDefense def = m_Cmd.GetDefense();
		if (n == 2)
		{
			if (def)
				def.RequestThreatOP(m_Cmd, d.m_vPos, objPos, now);
			DCO_Perf.Count("threat:nai");
			Log(string.Format("threat_nai requested=1 obj=%1 sector=%2 count=%3 dossier=%4 pos=%5", obj.GetLogName(), s, n, d.m_iId, DCO_Util.XZ(d.m_vPos)));
		}
		else if (n == 3)
		{
			if (def)
				def.RequestApproachAmbush(m_Cmd, obj, d.m_vPos, Rank(d.m_sClass) >= 3, now, true);
			DCO_Perf.Count("threat:tai");
			Log(string.Format("threat_tai requested=1 obj=%1 sector=%2 count=%3 dossier=%4 pos=%5", obj.GetLogName(), s, n, d.m_iId, DCO_Util.XZ(d.m_vPos)));
		}
	}

	DCO_ThreatDossier Observe(CMD_ThreatCluster c, CMD_ThreatAssessment a, CMD_ThreatMission mission, float now)
	{
		vector pos = c.m_vCenterPos;
		string cls = ClassOf(a, c);
		DCO_ThreatDossier d = Find(pos, now, mission.m_iId);
		if (!d)
		{
			d = new DCO_ThreatDossier();
			d.m_iId = ++s_iNext;
			d.m_sClass = cls;
			d.m_vPos = pos;
			d.m_fFirstAt = now;
			d.m_fLastAt = now;
			m_aD.Insert(d);
			DCO_Perf.Count("threat:dossier_new");
			Log(string.Format("threat_dossier_new id=%1 class=%2 enemies=%3 pos=%4 mission=%5", d.m_iId, cls, a.m_iEnemies, DCO_Util.XZ(pos), mission.m_iId));
			NoteSector(d, a, now);
			CheckDecoy(d, c, a, now);
		}
		else
		{
			if (now - d.m_fLastAt > 120.0)
			{
				Log(string.Format("threat_dossier_back id=%1 class=%2 gap_s=%3 pos=%4", d.m_iId, d.m_sClass, Math.Round(now - d.m_fLastAt), DCO_Util.XZ(pos)));
				NoteSector(d, a, now);
			}
			if (Rank(cls) > Rank(d.m_sClass))
			{
				Log(string.Format("threat_dossier_class id=%1 from=%2 to=%3", d.m_iId, d.m_sClass, cls));
				d.m_sClass = cls;
			}
			d.m_vPos = pos;
			d.m_fLastAt = now;
		}
		d.m_iMaxEnemies = Math.Max(d.m_iMaxEnemies, a.m_iEnemies);
		d.m_fRatio = a.m_fRatio;
		d.m_bCanMatch = a.m_bCanMatch;
		d.m_bProtected = a.m_bEmergency || a.m_eAsset == CMD_EAssetKind.OBJECTIVE || c.m_eClusterLevel >= CMD_EThreatLevel.HIGH;
		d.m_iClaimMission = mission.m_iId;
		d.m_fClaimAt = now;
		mission.m_iDossier = d.m_iId;
		return d;
	}

	protected void CheckDecoy(DCO_ThreatDossier fresh, CMD_ThreatCluster c, CMD_ThreatAssessment a, float now)
	{
		bool real = a.m_bEmergency || c.m_eClusterLevel >= CMD_EThreatLevel.HIGH || (a.m_eAsset == CMD_EAssetKind.OBJECTIVE && c.m_eClusterLevel >= CMD_EThreatLevel.MEDIUM);
		if (!real)
			return;
		foreach (DCO_ThreatDossier x : m_aD)
		{
			if (x == fresh || x.m_bDecoy || x.m_bProtected || !x.m_Resp || now - x.m_fDrawAt > 300.0)
				continue;
			if (vector.DistanceXZ(x.m_vPos, fresh.m_vPos) < 400.0)
				continue;
			x.m_bDecoy = true;
			x.m_fDecoyAt = now;
			DCO_Perf.Count("threat:decoy_suspect");
			Log(string.Format("threat_decoy_suspect dossier=%1 class=%2 real=%3 real_class=%4 dist=%5 drawn_s=%6", x.m_iId, x.m_sClass, fresh.m_iId, fresh.m_sClass, Math.Round(vector.DistanceXZ(x.m_vPos, fresh.m_vPos)), Math.Round(now - x.m_fDrawAt)));
			Radio(x, "threat_decoy", "", "");
		}
	}

	protected static bool Confirmed(CMD_ThreatCluster c)
	{
		if (c.m_aMembers.Count() >= 2)
			return true;
		foreach (CMD_ThreatEntry e : c.m_aMembers)
		{
			if (e && e.m_fReportQuality >= 0.6)
				return true;
		}
		return false;
	}

	protected static void Mul(CMD_ThreatAssessment a, CMD_EThreatAction act, float f)
	{
		if (a.m_aScores.IsIndexValid(act))
			a.m_aScores[act] = a.m_aScores[act] * f;
	}

	protected static void Add(CMD_ThreatAssessment a, CMD_EThreatAction act, float v)
	{
		if (a.m_aScores.IsIndexValid(act))
			a.m_aScores[act] = a.m_aScores[act] + v;
	}

	void Bias(DCO_ThreatDossier d, CMD_ThreatAssessment a, CMD_ThreatCluster c, float now)
	{
		if (!d)
			return;
		a.m_iDossier = d.m_iId;
		string applied;
		foreach (int act, float until : d.m_mBan)
		{
			if (now < until)
			{
				Mul(a, act, 0.1);
				applied += "ban_" + CMD_ThreatMission.ActionName(act) + ",";
			}
		}
		if (d.m_iPrefer >= 0 && a.Score(d.m_iPrefer) > 0)
		{
			Add(a, d.m_iPrefer, 0.2);
			applied += "prefer,";
		}
		if (d.m_sMeans == "at")
			a.m_bNeedAT = true;
		if (d.m_sMeans == "at" || d.m_sMeans == "mortar" || d.m_sMeans == "mortar_flank" || d.m_sMeans == "contain" || d.m_bHPT)
			a.m_bForceArty = true;
		if (d.m_sMeans == "flank" || d.m_sMeans == "mortar_flank")
		{
			if (a.Score(CMD_EThreatAction.COUNTERATTACK) <= 0)
				Add(a, CMD_EThreatAction.COUNTERATTACK, 0.3);
			else
				Add(a, CMD_EThreatAction.COUNTERATTACK, 0.25);
		}
		if (d.m_sMeans == "hold")
			Add(a, CMD_EThreatAction.HOLD_DESTROY, 0.25);
		if (d.m_sMeans == "contain")
		{
			Mul(a, CMD_EThreatAction.HOLD_DESTROY, 0.3);
			Mul(a, CMD_EThreatAction.COUNTERATTACK, 0.3);
			Add(a, CMD_EThreatAction.MONITOR, 0.5);
		}
		if (d.m_bDecoy)
		{
			Mul(a, CMD_EThreatAction.HOLD_DESTROY, 0.2);
			Mul(a, CMD_EThreatAction.COUNTERATTACK, 0.2);
			Mul(a, CMD_EThreatAction.SUPPORT_OP, 0.2);
			Add(a, CMD_EThreatAction.MONITOR, 0.4);
			a.m_bForceArty = true;
			applied += "decoy,";
		}
		if (d.m_iEsc >= 1 && !Confirmed(c))
		{
			Mul(a, CMD_EThreatAction.HOLD_DESTROY, 0.3);
			Mul(a, CMD_EThreatAction.COUNTERATTACK, 0.3);
			Add(a, CMD_EThreatAction.RECON, 0.3);
			applied += "unconfirmed,";
		}
		if (!d.m_sMeans.IsEmpty())
			applied += "means_" + d.m_sMeans + ",";
		if (d.m_bHPT)
			applied += "hpt,";
		if (!applied.IsEmpty())
		{
			DCO_Perf.Count("threat:bias");
			if (DCO_Log.On(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG, "-", string.Format("threat_bias dossier=%1 class=%2 esc=%3 applied=%4 best=%5", d.m_iId, d.m_sClass, d.m_iEsc, applied, CMD_ThreatMission.ActionName(a.Best())));
		}
	}

	protected static bool Tracked(int act)
	{
		return act == CMD_EThreatAction.HOLD_DESTROY || act == CMD_EThreatAction.COUNTERATTACK || act == CMD_EThreatAction.SUPPORT_OP;
	}

	void Sync(DCO_ThreatDossier d, CMD_ThreatMission mission, float now)
	{
		if (!d)
			return;
		if (d.m_Resp && d.m_Resp.m_iAction != mission.m_eAction)
		{
			string drop;
			if (d.m_bDecoy)
				drop = "decoy_switch";
			else if (now - d.m_Resp.m_fAt < 60.0)
				drop = "short";
			if (drop.IsEmpty())
				Assess(d, now, true, "switch");
			else
			{
				DCO_Perf.Count("threat:resp_drop_" + drop);
				if (DCO_Log.On(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG))
					DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.DEBUG, "-", string.Format("threat_resp_drop dossier=%1 action=%2 why=%3 age_s=%4", d.m_iId, CMD_ThreatMission.ActionName(d.m_Resp.m_iAction), drop, Math.Round(now - d.m_Resp.m_fAt)));
				d.m_Resp = null;
			}
		}
		if (!d.m_Resp && Tracked(mission.m_eAction) && !mission.m_aGroups.IsEmpty())
		{
			d.m_Resp = new DCO_ThreatResp();
			d.m_Resp.m_iAction = mission.m_eAction;
			d.m_Resp.m_fAt = now;
			d.m_fDrawAt = now;
		}
		if (!d.m_Resp)
			return;
		foreach (DCO_GroupUtilityComponent g : mission.m_aGroups)
		{
			if (g && !d.m_Resp.m_aGroups.Contains(g))
			{
				d.m_Resp.m_aGroups.Insert(g);
				d.m_Resp.m_aStart.Insert(Math.Max(g.GetUnitCount(), 1));
			}
		}
		if (now - d.m_Resp.m_fAt >= 300.0)
			Assess(d, now, true, "window");
	}

	void OnGone(int id, float now)
	{
		DCO_ThreatDossier d = Get(id);
		if (d && d.m_Resp)
			Assess(d, now, false, "gone");
	}

	protected void Assess(DCO_ThreatDossier d, float now, bool present, string why)
	{
		DCO_ThreatResp r = d.m_Resp;
		d.m_Resp = null;
		if (!r)
			return;
		int start;
		int alive;
		int lost;
		foreach (int i, DCO_GroupUtilityComponent g : r.m_aGroups)
		{
			start += r.m_aStart[i];
			if (g && g.GetUnitCount() > 0)
				alive += g.GetUnitCount();
			else
				lost++;
		}
		float loss = 0;
		if (start > 0)
			loss = 1.0 - alive / (start * 1.0);
		int ek = DCO_CmdLearn.DeathsNear(d.m_vPos, 250.0, m_Cmd.GetCommanderFactionKey(), r.m_fAt);
		string res;
		if (lost >= 1 || loss >= 0.33)
			res = "heavy_loss";
		else if (ek >= Math.Max(1, Math.Round(0.3 * d.m_iMaxEnemies)))
			res = "effective";
		else if (!present && ek >= 1)
			res = "effective";
		else if (!present)
			res = "inconclusive";
		else if (ek == 0 && loss <= 0)
			res = "inconclusive";
		else
			res = "ineffective";
		if (res == "ineffective")
		{
			d.m_iIneff++;
			if (d.m_iIneff < 2)
				res = "ineffective_1st";
		}
		else
			d.m_iIneff = 0;
		d.m_iGroupsKilled += lost;
		DCO_Perf.Count("threat:assess_" + res);
		Log(string.Format("threat_assess dossier=%1 class=%2 action=%3 result=%4 why=%5 enemy_deaths=%6 own_loss_pct=%7 groups_lost=%8 dur_s=%9", d.m_iId, d.m_sClass, CMD_ThreatMission.ActionName(r.m_iAction), res, why, ek, Math.Round(loss * 100.0), lost, Math.Round(now - r.m_fAt)));
		Adapt(d, r.m_iAction, res, now);
	}

	protected string ChangeMeans(DCO_ThreatDossier d, int act)
	{
		if (d.m_sClass == "armor" || d.m_sClass == "veh")
			return "at";
		if (d.m_sClass == "inf_mgat")
			return "mortar_flank";
		if (d.m_sClass == "sniper")
			return "mortar";
		if (d.m_sClass == "arty")
			return "flank";
		if (act == CMD_EThreatAction.COUNTERATTACK)
			return "hold";
		return "flank";
	}

	protected void Adapt(DCO_ThreatDossier d, int act, string res, float now)
	{
		string old = d.m_sMeans;
		if (res == "effective")
		{
			d.m_iPrefer = act;
			d.m_mBan.Remove(act);
		}
		else if (res == "ineffective")
		{
			d.m_iPrefer = -1;
			d.m_sMeans = ChangeMeans(d, act);
			d.m_iEsc = Math.Max(d.m_iEsc, 1);
			d.m_fLossAt = now;
		}
		else if (res == "heavy_loss")
		{
			d.m_iPrefer = -1;
			d.m_mBan.Set(act, now + 900.0);
			d.m_iEsc++;
			d.m_fLossAt = now;
			if (d.m_fRatio >= 1.5 && d.m_bCanMatch)
				d.m_sMeans = ChangeMeans(d, act);
			else
				d.m_sMeans = "contain";
		}
		if (d.m_iGroupsKilled >= 2 && !d.m_bHPT)
		{
			d.m_bHPT = true;
			DCO_Perf.Count("threat:hpt");
			Log(string.Format("threat_hpt dossier=%1 class=%2 groups_killed=%3 pos=%4", d.m_iId, d.m_sClass, d.m_iGroupsKilled, DCO_Util.XZ(d.m_vPos)));
			Radio(d, "threat_hpt", "", "");
		}
		if (old != d.m_sMeans)
		{
			DCO_Perf.Count("threat:escalate");
			string o = old;
			if (o.IsEmpty())
				o = "-";
			Log(string.Format("threat_escalate dossier=%1 class=%2 from=%3 to=%4 reason=%5 action=%6 esc=%7 ratio=%8", d.m_iId, d.m_sClass, o, d.m_sMeans, res, CMD_ThreatMission.ActionName(act), d.m_iEsc, d.m_fRatio.ToString(-1, 2)));
			Radio(d, "threat_adapt", CMD_ThreatMission.ActionKey(act), "@means_" + d.m_sMeans);
		}
	}

	void Tick(float now)
	{
		for (int i = m_aD.Count() - 1; i >= 0; i--)
		{
			DCO_ThreatDossier d = m_aD[i];
			if (d.m_bDecoy && now - d.m_fDecoyAt > 900.0)
			{
				d.m_bDecoy = false;
				Log(string.Format("threat_decoy_clear dossier=%1", d.m_iId));
			}
			if (d.m_iEsc > 0 && !d.m_Resp && now - d.m_fLossAt > 600.0)
			{
				d.m_iEsc--;
				d.m_fLossAt = now;
				if (d.m_iEsc == 0)
				{
					d.m_sMeans = "";
					d.m_mBan.Clear();
				}
				DCO_Perf.Count("threat:deescalate");
				Log(string.Format("threat_deescalate dossier=%1 class=%2 esc=%3", d.m_iId, d.m_sClass, d.m_iEsc));
			}
			if (d.m_Resp && now - d.m_fLastAt > 120.0)
				Assess(d, now, false, "orphan");
			if (d.m_bHPT && now - d.m_fLossAt > 1200.0)
			{
				d.m_bHPT = false;
				d.m_iGroupsKilled = 0;
				Log(string.Format("threat_hpt_clear dossier=%1 class=%2", d.m_iId, d.m_sClass));
			}
			if (!d.m_Resp && now - d.m_fLastAt > 1800.0)
				m_aD.Remove(i);
		}
	}

	protected bool CapLogOnce(int mission, string why)
	{
		string last;
		if (m_mCapLog.Find(mission, last) && last == why)
			return false;
		m_mCapLog.Set(mission, why);
		return true;
	}

	bool CapOk(CMD_ThreatMission mission, CMD_ThreatAssessment a, DCO_GroupUtilityComponent g, bool emergency)
	{
		if (!mission || !a || !g || emergency || a.m_bEmergency)
			return true;
		int sent;
		foreach (DCO_GroupUtilityComponent mg : mission.m_aGroups)
		{
			if (mg)
				sent += mg.GetUnitCount();
		}
		float need = Math.Max(8.0, a.m_fEnemy * 2.5);
		if (sent >= need)
		{
			DCO_Perf.Count("threat:cap_per_threat");
			if (CapLogOnce(mission.m_iId, "per_threat"))
				Log(string.Format("threat_cap mission=%1 reason=per_threat sent=%2 need=%3 dossier=%4", mission.m_iId, sent, Math.Round(need), a.m_iDossier));
			return false;
		}
		if (a.m_eAsset == CMD_EAssetKind.OBJECTIVE)
			return true;
		int committed;
		int total;
		foreach (DCO_GroupUtilityComponent og : m_Cmd.GetOwnedGroups())
		{
			if (!og || og.IsPlayerGroup())
				continue;
			total += og.GetUnitCount();
			if (og.m_iDCOThreatAct == CMD_EThreatAction.HOLD_DESTROY || og.m_iDCOThreatAct == CMD_EThreatAction.COUNTERATTACK)
				committed += og.GetUnitCount();
		}
		if (committed > 0 && committed + g.GetUnitCount() > total / 3.0)
		{
			DCO_Perf.Count("threat:cap_non_main");
			if (CapLogOnce(mission.m_iId, "non_main"))
				Log(string.Format("threat_cap mission=%1 reason=non_main committed=%2 total=%3 dossier=%4", mission.m_iId, committed, total, a.m_iDossier));
			return false;
		}
		return true;
	}

	protected void Radio(DCO_ThreatDossier d, string key, string oldAct, string means)
	{
		DCO_GlobalAIComponent cfg = DCO_GlobalAIComponent.GetInstance();
		float radius = 800;
		if (cfg)
			radius = cfg.GetRadioRadius();
		array<int> ids = {};
		DCO_PlayerAwareness.CollectPlayerGroupsNear(m_Cmd.GetCommanderFactionKey(), d.m_vPos, radius, ids);
		if (ids.IsEmpty())
			return;
		string type = "@thr_infantry";
		if (d.m_sClass == "armor")
			type = "@thr_armor";
		else if (d.m_sClass == "veh")
			type = "@thr_motorized";
		array<string> params = DCO_Radio.P("type", type, "grid", DCO_PlayerComms.Grid(d.m_vPos), "old", oldAct, "means", means);
		foreach (int id : ids)
			DCO_Radio.Group(id, "THREAT", key, DCO_ERadioKind.WARNING, params, d.m_bHPT);
	}
}
