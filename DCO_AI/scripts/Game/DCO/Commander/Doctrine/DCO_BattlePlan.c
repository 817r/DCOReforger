enum DCO_EDoctrineGate
{
	GO,
	RECON,
	WAIT,
	SKIP,
	MTC
}

class DCO_BattleState
{
	float m_fReconStart;
	bool m_bReconDone;
	float m_fWaitStart;
	int m_iPlanBand;
	bool m_bPlanActive;
	bool m_bRetreated;
	string m_sPhase;
	float m_fPhaseLogAt;
	int m_iReconRoll;
	int m_iStartUnits;

	void DCO_BattleState()
	{
		m_fReconStart = -1;
		m_iReconRoll = -1;
		m_fWaitStart = -1;
		m_iPlanBand = DCO_EForceBand.UNKNOWN;
		m_fPhaseLogAt = -1000;
	}
}

class DCO_BattlePlanner
{
	protected ref map<CMD_AICommanderObjectiveComponent, ref DCO_BattleState> m_mState = new map<CMD_AICommanderObjectiveComponent, ref DCO_BattleState>();

	DCO_BattleState State(CMD_AICommanderObjectiveComponent obj)
	{
		DCO_BattleState st = m_mState.Get(obj);
		if (!st)
		{
			st = new DCO_BattleState();
			m_mState.Set(obj, st);
		}
		return st;
	}

	void Reset(CMD_AICommanderObjectiveComponent obj)
	{
		m_mState.Remove(obj);
	}

	static int GovernedMask()
	{
		return DCO_CommanderTactics.Bit(DCO_ETactic.FIRE_AND_MANEUVER) | DCO_CommanderTactics.Bit(DCO_ETactic.FLANKING) | DCO_CommanderTactics.Bit(DCO_ETactic.PINCER) | DCO_CommanderTactics.Bit(DCO_ETactic.HAMMER_AND_ANVIL) | DCO_CommanderTactics.Bit(DCO_ETactic.RAID) | DCO_CommanderTactics.Bit(DCO_ETactic.ATTACK_BY_FIRE) | DCO_CommanderTactics.Bit(DCO_ETactic.SIEGE);
	}

	static int AttackMask(DCO_Doctrine d, int band)
	{
		int m = 0;
		if (d.Battle(band, DCO_EBattleCol.DIRECT))
			m |= DCO_CommanderTactics.Bit(DCO_ETactic.FIRE_AND_MANEUVER);
		if (d.Battle(band, DCO_EBattleCol.FLANK))
			m |= DCO_CommanderTactics.Bit(DCO_ETactic.FLANKING) | DCO_CommanderTactics.Bit(DCO_ETactic.PINCER) | DCO_CommanderTactics.Bit(DCO_ETactic.HAMMER_AND_ANVIL);
		if (d.Battle(band, DCO_EBattleCol.HITRUN))
			m |= DCO_CommanderTactics.Bit(DCO_ETactic.RAID);
		if (d.Battle(band, DCO_EBattleCol.ABF))
			m |= DCO_CommanderTactics.Bit(DCO_ETactic.ATTACK_BY_FIRE) | DCO_CommanderTactics.Bit(DCO_ETactic.SIEGE);
		return m;
	}

	static string MaskNames(int mask)
	{
		string text;
		for (int t = 0; t <= DCO_ETactic.DELAY; t++)
		{
			if ((mask & DCO_CommanderTactics.Bit(t)) == 0)
				continue;
			if (!text.IsEmpty())
				text += ",";
			text += DCO_CommanderTactics.Name(t);
		}
		if (text.IsEmpty())
			return "none";
		return text;
	}

	static float WaitTimeout(AICommander_BaseComponent cmd)
	{
		float baseWait = 240;
		DCO_Doctrine d = cmd.GetDoctrine();
		if (d)
			baseWait = d.m_fWaitTimeout;
		if (cmd.GetDecision() == DCO_EDecision.FAST)
			return baseWait * 0.5;
		if (cmd.GetDecision() == DCO_EDecision.SLOW)
			return baseWait * 1.5;
		return baseWait;
	}

	static float Tempo(AICommander_BaseComponent cmd)
	{
		if (cmd.DCO_K2())
			return cmd.DCO_K2Tempo();
		if (DCO_Ctx.TuneForCmd(cmd, "cmd.aggr_tempo", 0) <= 0)
			return 1.0;
		return Math.Clamp(cmd.GetAggression() * 2.0 * DCO_MettTable.Slider(cmd), DCO_Ctx.TuneForCmd(cmd, "cmd.tempo_min", 0.75), DCO_Ctx.TuneForCmd(cmd, "cmd.tempo_max", 2.0));
	}

	static float StagingMargin(AICommander_BaseComponent cmd)
	{
		float m = 180;
		if (cmd.GetDecision() == DCO_EDecision.FAST)
			m = 60;
		else if (cmd.GetDecision() == DCO_EDecision.SLOW)
			m = 300;
		if (cmd.DCO_K2())
			return Math.Max(m / Tempo(cmd), Math.Min(m, DCO_Ctx.TuneForCmd(cmd, "agg.k2_staging_floor_s", 30.0)));
		return m / Tempo(cmd);
	}

	static string TraitFor(string phase)
	{
		if (phase == "recon" || phase == "mtc")
			return "intel";
		if (phase == "barrage")
			return "support";
		if (phase == "wait" || phase == "withdraw" || phase == "fallback")
			return "decision_courage";
		if (phase == "staged_prep")
			return "decision";
		return "aggression";
	}

	void Phase(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, string phase, string reason, float now)
	{
		DCO_BattleState st = State(obj);
		if (st.m_sPhase == phase && now - st.m_fPhaseLogAt < 60)
			return;
		if (st.m_sPhase != phase)
			DCO_Perf.Count("doctrine:phase:" + phase);
		st.m_sPhase = phase;
		st.m_fPhaseLogAt = now;
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), "battle_phase cmd=" + cmd.GetCommanderUID() + " obj=" + obj.GetLogName() + " phase=" + phase + " reason=" + reason + " trait=" + TraitFor(phase));
	}

	int Gate(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, DCO_DoctrineRuntime rt, float now, out int tacticMask)
	{
		tacticMask = -1;
		DCO_Doctrine d = cmd.GetDoctrine();
		if (!d || !rt)
			return DCO_EDoctrineGate.GO;
		DCO_BattleState st = State(obj);
		if (DCO_TacticTest.Forces(cmd, obj))
		{
			DCO_TacticTest.NoteBypass(cmd, obj, "doctrine_gate");
			if (DCO_TacticTest.ForceSelect(cmd, obj))
				tacticMask = DCO_CommanderTactics.Bit(DCO_TacticTest.Tactic());
			st.m_iPlanBand = DCO_EForceBand.STRONG;
			st.m_bReconDone = true;
			st.m_fWaitStart = -1;
			if (!st.m_bPlanActive)
			{
				st.m_iStartUnits = 0;
				Phase(cmd, obj, "attack", "test_forced", now);
			}
			st.m_bPlanActive = true;
			st.m_bRetreated = false;
			return DCO_EDoctrineGate.GO;
		}
		int allowed = cmd.GetAllowedTactics();
		int band = rt.GetBand(obj);
		if (st.m_fWaitStart >= 0 && band != DCO_EForceBand.UNKNOWN)
		{
			DCO_ForceEval eval = rt.GetEval(obj);
			float minutes = (now - st.m_fWaitStart) / 60.0;
			if (eval && eval.m_fPosture > 1.0)
			{
				float required = 1 + 0.1 * minutes;
				int waitBand = DCO_DoctrineRuntime.BandOf(eval.m_fEff / required * (1 + eval.m_fShift));
				if (waitBand > band)
					band = waitBand;
			}
		}

		bool reconWanted = d.Battle(band, DCO_EBattleCol.RECON);
		if (!reconWanted)
		{
			if (st.m_iReconRoll < 0)
			{
				st.m_iReconRoll = 0;
				if (Math.RandomFloat01() < cmd.GetTraitIntel() * 0.5)
					st.m_iReconRoll = 1;
				DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), "doctrine_choice cmd=" + cmd.GetCommanderUID() + " obj=" + obj.GetLogName() + " column=recon band=" + DCO_Doctrine.BandName(band) + " intel=" + cmd.GetTraitIntel().ToString(-1, 1) + " recon=" + st.m_iReconRoll.ToString());
			}
			reconWanted = st.m_iReconRoll == 1;
		}
		if (reconWanted && !st.m_bReconDone)
		{
			bool reconOk = band != DCO_EForceBand.UNKNOWN && obj.IsReconArrived(cmd.GetCommanderFactionKey(), now);
			if (!reconOk)
			{
				if (st.m_fReconStart < 0)
					st.m_fReconStart = now;
				float reconLimit = d.m_fReconTimeout / Tempo(cmd);
				if (cmd.DCO_K2())
					reconLimit = Math.Max(reconLimit, Math.Min(d.m_fReconTimeout, DCO_Ctx.TuneForCmd(cmd, "agg.k2_recon_floor_s", 90.0)));
				if (now - st.m_fReconStart < reconLimit)
				{
					Phase(cmd, obj, "recon", "band_" + DCO_Doctrine.BandName(band) + "_limit_" + Math.Round(reconLimit).ToString(), now);
					return DCO_EDoctrineGate.RECON;
				}
				if (band == DCO_EForceBand.UNKNOWN)
				{
					st.m_bReconDone = true;
					if ((allowed & DCO_CommanderTactics.Bit(DCO_ETactic.MOVEMENT_TO_CONTACT)) != 0)
					{
						tacticMask = DCO_CommanderTactics.Bit(DCO_ETactic.MOVEMENT_TO_CONTACT);
						Phase(cmd, obj, "mtc", "recon_timeout", now);
						LogDecision(cmd, obj, band, tacticMask, "MOVEMENT_TO_CONTACT", 0);
						return DCO_EDoctrineGate.MTC;
					}
					return WaitOrSkip(cmd, obj, d, band, st, now, "no_intel");
				}
			}
			st.m_bReconDone = true;
		}

		if (band == DCO_EForceBand.UNKNOWN && st.m_bReconDone && (allowed & DCO_CommanderTactics.Bit(DCO_ETactic.MOVEMENT_TO_CONTACT)) != 0 && reconWanted)
		{
			tacticMask = DCO_CommanderTactics.Bit(DCO_ETactic.MOVEMENT_TO_CONTACT);
			Phase(cmd, obj, "mtc", "still_unknown", now);
			return DCO_EDoctrineGate.MTC;
		}

		int attack = AttackMask(d, band) & allowed;
		if (attack == 0)
			return WaitOrSkip(cmd, obj, d, band, st, now, "no_attack_type");

		if (st.m_fWaitStart >= 0)
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), "wait_state cmd=" + cmd.GetCommanderUID() + " obj=" + obj.GetLogName() + " exit=reinforcement waited_s=" + Math.Round(now - st.m_fWaitStart).ToString() + " band=" + DCO_Doctrine.BandName(band));
			st.m_fWaitStart = -1;
		}
		tacticMask = attack | (allowed & ~GovernedMask());
		st.m_iPlanBand = band;
		if (!st.m_bPlanActive)
		{
			st.m_iStartUnits = 0;
			rt.GetTerrain().OnAttackPlanned(cmd, obj, now);
		}
		st.m_bPlanActive = true;
		st.m_bRetreated = false;
		return DCO_EDoctrineGate.GO;
	}

	protected int WaitOrSkip(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, DCO_Doctrine d, int band, DCO_BattleState st, float now, string reason)
	{
		if (!d.Battle(band, DCO_EBattleCol.WAIT))
		{
			Phase(cmd, obj, "skip", reason + "_band_" + DCO_Doctrine.BandName(band), now);
			LogDecision(cmd, obj, band, 0, "skip", 0);
			st.m_bReconDone = false;
			st.m_iReconRoll = -1;
			st.m_fReconStart = -1;
			return DCO_EDoctrineGate.SKIP;
		}
		if (st.m_fWaitStart < 0)
			st.m_fWaitStart = now;
		float waited = now - st.m_fWaitStart;
		float required = 1 + 0.1 * waited / 60.0;
		if (waited > WaitTimeout(cmd))
		{
			DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), "wait_state cmd=" + cmd.GetCommanderUID() + " obj=" + obj.GetLogName() + " exit=timeout waited_s=" + Math.Round(waited).ToString() + " req_ratio=" + required.ToString(-1, 2));
			st.m_fWaitStart = -1;
			st.m_bReconDone = false;
			st.m_iReconRoll = -1;
			st.m_fReconStart = -1;
			Phase(cmd, obj, "skip", "wait_timeout", now);
			return DCO_EDoctrineGate.SKIP;
		}
		Phase(cmd, obj, "wait", reason + "_band_" + DCO_Doctrine.BandName(band) + "_req_" + required.ToString(-1, 2), now);
		return DCO_EDoctrineGate.WAIT;
	}

	void LogDecision(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, int band, int mask, string chosen, float reserve)
	{
		DCO_Log.Write(DCO_ELogChannel.CMD, DCO_ELogLevel.INFO, DCO_Log.IdCmd(cmd), "doctrine_decision cmd=" + cmd.GetCommanderUID() + " obj=" + obj.GetLogName() + " band=" + DCO_Doctrine.BandName(band) + " allowed=" + MaskNames(mask) + " chosen=" + chosen + " reserve=" + Math.Round(reserve).ToString());
	}

	DCO_BattleState Peek(CMD_AICommanderObjectiveComponent obj)
	{
		return m_mState.Get(obj);
	}

	bool UseStaging(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, bool fallback)
	{
		DCO_Doctrine d = cmd.GetDoctrine();
		DCO_BattleState st = m_mState.Get(obj);
		if (!d || !st || !st.m_bPlanActive)
			return fallback;
		bool staged = d.Battle(st.m_iPlanBand, DCO_EBattleCol.STAGED);
		DCO_Perf.Count("doc:staging_" + DCO_Util.B01(staged).ToString());
		return staged;
	}

	int PrepMode(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj)
	{
		DCO_Doctrine d = cmd.GetDoctrine();
		DCO_BattleState st = m_mState.Get(obj);
		if (DCO_TacticTest.Forces(cmd, obj))
		{
			DCO_TacticTest.NoteBypass(cmd, obj, "prep_mode");
			return 0;
		}
		if (!d || !st || !st.m_bPlanActive)
			return -1;
		if (d.Battle(st.m_iPlanBand, DCO_EBattleCol.BARRAGE))
			return 1;
		return -1;
	}

	bool ReserveRequired(AICommander_BaseComponent cmd)
	{
		DCO_Doctrine d = cmd.GetDoctrine();
		if (!d)
			return false;
		foreach (CMD_AICommanderObjectiveComponent obj, DCO_BattleState st : m_mState)
		{
			if (!st || !st.m_bPlanActive || DCO_TacticTest.Forces(cmd, obj))
				continue;
			if (d.Battle(st.m_iPlanBand, DCO_EBattleCol.RESERVE))
				return true;
			if (d.Battle(st.m_iPlanBand, DCO_EBattleCol.RESERVE_ADVISED) && cmd.DCO_TraitAggressionPlan() < 1.5)
				return true;
		}
		return false;
	}

	int CheckAttack(AICommander_BaseComponent cmd, CMD_AICommanderObjectiveComponent obj, DCO_DoctrineRuntime rt, float now)
	{
		DCO_Doctrine d = cmd.GetDoctrine();
		DCO_BattleState st = m_mState.Get(obj);
		if (!d || !st || !st.m_bPlanActive || st.m_bRetreated || !rt)
			return 0;
		int band = rt.GetBand(obj);
		int units = 0;
		foreach (DCO_GroupUtilityComponent g : cmd.GetOwnedGroups())
		{
			if (g && g.GetOwner() && !g.IsPlayerGroup() && g.GetGroupObjective() == obj)
				units += g.GetUnitCount();
		}
		if (units > st.m_iStartUnits)
			st.m_iStartUnits = units;
		float loss = 0;
		if (st.m_iStartUnits > 0)
			loss = 1 - 1.0 * units / Math.Max(st.m_iStartUnits, 1.0);
		float retreatLoss = d.m_fRetreatLoss;
		if (retreatLoss > 0 && cmd.DCO_K2())
			retreatLoss = Math.Max(Math.Min(retreatLoss + DCO_Ctx.TuneForCmd(cmd, "agg.k2_retreat_shift", 0.05) * Math.Log10(cmd.DCO_K2Slider()), Math.Max(retreatLoss, 0.6)), retreatLoss * 0.5);
		else if (retreatLoss > 0 && cmd.DCO_AggProfile())
			retreatLoss = Math.Min(retreatLoss * Math.Pow(cmd.DCO_AggrA(), 0.3), Math.Max(retreatLoss, 0.6));
		bool lossHit = retreatLoss > 0 && st.m_iStartUnits >= 4 && loss >= retreatLoss;
		string reason = "band_weaker";
		if (lossHit)
			reason = "losses_" + Math.Round(loss * 100).ToString();
		else
		{
			if (band != DCO_EForceBand.WEAKER || st.m_iPlanBand >= DCO_EForceBand.WEAKER)
				return 0;
			if ((AttackMask(d, band) & cmd.GetAllowedTactics()) != 0)
				return 0;
		}
		bool canFallback = d.Battle(band, DCO_EBattleCol.FALLBACK);
		bool canWithdraw = d.Battle(band, DCO_EBattleCol.WITHDRAW) || (lossHit && !canFallback);
		if (!canFallback && !canWithdraw)
			return 0;
		st.m_bRetreated = true;
		st.m_bPlanActive = false;
		bool preferFallback = cmd.DCO_TraitAggressionPlan() >= 1 || cmd.GetCourage() >= DCO_ECourage.BOLD;
		if (canFallback && (preferFallback || !canWithdraw))
		{
			Phase(cmd, obj, "fallback", reason, now);
			return 2;
		}
		Phase(cmd, obj, "withdraw", reason, now);
		return 1;
	}

	float StagingDeadline(AICommander_BaseComponent cmd, float maxDist, float now, float fallbackWait)
	{
		if (!cmd.GetDoctrine())
			return now + maxDist / 1.5 + fallbackWait;
		return now + maxDist / 1.5 * 1.2 + StagingMargin(cmd);
	}

	void OnCaptured(CMD_AICommanderObjectiveComponent obj)
	{
		DCO_BattleState st = m_mState.Get(obj);
		if (st)
			st.m_bPlanActive = false;
	}
}
