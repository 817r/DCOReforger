class DCO_CensusSide
{
	int n;
	float supp, fly, end, shots, inj, total;
	int tSafe, tVig, tAlert, tThreat;
	int s0, s1, s2;
	float mSupp, mEnd, mEv, mRel, mCond, mBoost;
	int mCalm, mTense, mShaken, mPanic, mBreak, mSpirited;

	void Add(SCR_AIUtilityComponent u)
	{
		n++;
		SCR_AIThreatSystem ts = u.m_ThreatSystem;
		if (ts)
		{
			float s, sh, ij, en;
			ts.GetThreatValues(s, sh, ij, en);
			supp += s;
			shots += sh;
			inj += ij;
			end += en;
			fly += ts.DCO_GetFlyby();
			total += ts.GetThreatMeasure();
			switch (ts.GetState())
			{
				case EAIThreatState.SAFE: tSafe++; break;
				case EAIThreatState.VIGILANT: tVig++; break;
				case EAIThreatState.ALERTED: tAlert++; break;
				case EAIThreatState.THREATENED: tThreat++; break;
			}
		}
		int lvl = DCO_Stress.Level(u);
		if (lvl == 2)
			s2++;
		else if (lvl == 1)
			s1++;
		else
			s0++;
		DCO_AIMoraleSystem ms = u.GetMoraleSystem();
		if (ms)
		{
			float a, b, c, d, e, f;
			ms.DCO_GetComponents(a, b, c, d, e, f);
			mSupp += a;
			mEnd += b;
			mEv += c;
			mRel += d;
			mCond += e;
			mBoost += f;
			switch (ms.GetState())
			{
				case moraleState.CALM: mCalm++; break;
				case moraleState.TENSE: mTense++; break;
				case moraleState.SHAKEN: mShaken++; break;
				case moraleState.PANIC: mPanic++; break;
				case moraleState.BREAK: mBreak++; break;
			}
			if (ms.IsSpirited())
				mSpirited++;
		}
	}

	static string Avg(float v, int cnt)
	{
		return (v / Math.Max(cnt, 1)).ToString(-1, 2);
	}
}

class DCO_Census
{
	protected static bool s_bOn;
	protected static float s_fInterval;
	protected static float s_fTimer;
	protected static int s_iFrames;
	protected static float s_fMaxFrame;
	protected static int s_iPrevMemKB;
	protected static int s_iStartMemKB;
	protected static float s_fShareSum;
	protected static int s_iShareSamples;
	protected static float s_fSharePeak;
	protected static int s_iPeakAI;
	protected static int s_iPeakPlayers;
	protected static int s_iPeakGroupsContact;
	protected static ref map<string, int> s_mSpikeTops;

	static bool IsOn()					{ return s_bOn; }

	static float GetInterval()
	{
		if (s_fInterval <= 0)
			return 30;
		return s_fInterval;
	}

	static void SetInterval(float seconds)
	{
		s_fInterval = Math.Clamp(seconds, 10, 300);
	}

	static void SetEnabled(bool on)
	{
		if (!Replication.IsServer() || on == s_bOn)
			return;

		if (!on)
		{
			WriteSummary("off");
			s_bOn = false;
			DCO_Perf.SetCensus(false);
			return;
		}

		s_bOn = true;
		s_fTimer = 0;
		s_iFrames = 0;
		s_fMaxFrame = 0;
		s_fShareSum = 0;
		s_iShareSamples = 0;
		s_fSharePeak = 0;
		s_iPeakAI = 0;
		s_iPeakPlayers = 0;
		s_iPeakGroupsContact = 0;
		s_iStartMemKB = System.MemoryAllocationKB();
		s_iPrevMemKB = s_iStartMemKB;
		if (!s_mSpikeTops)
			s_mSpikeTops = new map<string, int>();
		s_mSpikeTops.Clear();
		DCO_Perf.SetCensus(true);
		Write(string.Format("CENSUS start interval=%1s mem_kb=%2", Math.Round(GetInterval()), s_iStartMemKB));
	}

	static void OnShutdown()
	{
		if (s_bOn)
			WriteSummary("end");
		s_bOn = false;
	}

	static void Tick(float timeSlice)
	{
		if (!s_bOn)
			return;

		s_iFrames++;
		s_fTimer += timeSlice;
		if (timeSlice > s_fMaxFrame)
			s_fMaxFrame = timeSlice;

		if (s_fTimer < GetInterval())
			return;

		WriteInterval();
		s_fTimer = 0;
		s_iFrames = 0;
		s_fMaxFrame = 0;
		DCO_Perf.ResetCensus();
	}

	static void Spike(float timeSlice, string top, int shots, int callLater, int queries, int traces)
	{
		if (!s_bOn)
			return;

		if (s_mSpikeTops)
			s_mSpikeTops.Set(top, s_mSpikeTops.Get(top) + 1);
		Write(string.Format("CENSUS spike frame_ms=%1 shot_react=%2 calllater=%3 queries=%4 traces=%5 top=%6", Math.Round(timeSlice * 1000), shots, callLater, queries, traces, top));
	}

	static bool IsTopLevel(string name)
	{
		if (name == "cmd_contact_intake")
			return false;
		if (name.StartsWith("cmd_") || name.StartsWith("grp_") || name.StartsWith("bt_"))
			return true;
		return name == "agent_eval" || name == "agent_long_range" || name == "aim_pick" || name == "expose"
			|| name == "contact_sharing" || name == "medic_dispatch" || name == "veh_fire_position";
	}

	protected static void WriteInterval()
	{
		float windowMs = s_fTimer * 1000;
		float fpsAvg = 0;
		if (s_fTimer > 0)
			fpsAvg = 1.0 * s_iFrames / s_fTimer;
		float fpsMin = 0;
		if (s_fMaxFrame > 0)
			fpsMin = 1.0 / s_fMaxFrame;

		int dcoMs = 0;
		array<string> names = {};
		array<int> totals = {};
		foreach (string name, DCO_PerfSection s : DCO_Perf.GetSections())
		{
			if (s.m_iCensusCalls <= 0)
				continue;
			if (IsTopLevel(name))
				dcoMs += s.m_iCensusTotal;
			int at = 0;
			while (at < totals.Count() && totals[at] >= s.m_iCensusTotal)
				at++;
			names.InsertAt(name, at);
			totals.InsertAt(s.m_iCensusTotal, at);
		}

		float share = 0;
		if (windowMs > 0)
			share = dcoMs * 100.0 / windowMs;
		s_fShareSum += share;
		s_iShareSamples++;
		if (share > s_fSharePeak)
			s_fSharePeak = share;

		string top;
		for (int i = 0; i < names.Count() && i < 5; i++)
			top += string.Format(" %1=%2ms", names[i], totals[i]);

		Write(string.Format("CENSUS load fps_avg=%1 fps_min=%2 frame_max_ms=%3 dco_ms=%4 dco_ms_share=%5 top:%6",
			fpsAvg.ToString(-1, 1), fpsMin.ToString(-1, 1), Math.Round(s_fMaxFrame * 1000), dcoMs, share.ToString(-1, 1), top));

		WritePopulation();
		WriteRates();
		WriteQueues();
	}

	protected static void WritePopulation()
	{
		array<vector> playerPos = {};
		array<int> pids = {};
		PlayerManager pm = GetGame().GetPlayerManager();
		if (pm)
		{
			pm.GetPlayers(pids);
			foreach (int pid : pids)
			{
				IEntity pe = pm.GetPlayerControlledEntity(pid);
				if (pe)
					playerPos.Insert(pe.GetOrigin());
			}
		}

		int aiTotal = 0;
		int aiContact = 0;
		int aiVehicle = 0;
		int aiNear = 0;
		int aiFar = 0;
		int aiLocked = 0;
		int aiLeaning = 0;
		int mCalm, mTense, mShaken, mPanic, mBreak, mSpirited;
		set<AIGroup> groups = new set<AIGroup>();
		map<string, ref DCO_CensusSide> sides = new map<string, ref DCO_CensusSide>();
		set<IEntity> vehicles = new set<IEntity>();

		AIWorld aiWorld = GetGame().GetAIWorld();
		if (aiWorld)
		{
			array<AIAgent> agents = {};
			aiWorld.GetAIAgents(agents);
			foreach (AIAgent agent : agents)
			{
				if (!SCR_ChimeraAIAgent.Cast(agent))
					continue;
				IEntity ent = agent.GetControlledEntity();
				if (!ent)
					continue;

				aiTotal++;
				DCO_ShotCounter.Track(ent);
				AIGroup parent = agent.GetParentGroup();
				if (parent)
					groups.Insert(parent);

				SCR_AIUtilityComponent util = SCR_AIUtilityComponent.Cast(agent.FindComponent(SCR_AIUtilityComponent));
				if (util)
				{
					string sideKey = "none";
					FactionAffiliationComponent sideFac = FactionAffiliationComponent.Cast(ent.FindComponent(FactionAffiliationComponent));
					if (sideFac && sideFac.GetAffiliatedFaction())
						sideKey = sideFac.GetAffiliatedFaction().GetFactionKey();
					DCO_CensusSide side = sides.Get(sideKey);
					if (!side)
					{
						side = new DCO_CensusSide();
						sides.Set(sideKey, side);
					}
					side.Add(util);
					if (util.m_iDCO_LOD > 0)
						aiLocked++;
					if (util.m_CombatComponent && util.m_CombatComponent.GetCurrentTarget())
						aiContact++;
					DCO_AIMoraleSystem ms = util.GetMoraleSystem();
					if (ms)
					{
						switch (ms.GetState())
						{
							case moraleState.CALM: mCalm++; break;
							case moraleState.TENSE: mTense++; break;
							case moraleState.SHAKEN: mShaken++; break;
							case moraleState.PANIC: mPanic++; break;
							case moraleState.BREAK: mBreak++; break;
						}
						if (ms.IsSpirited())
							mSpirited++;
					}
				}

				ChimeraCharacter chr = ChimeraCharacter.Cast(ent);
				if (chr && chr.GetCharacterController() && Math.AbsFloat(chr.GetCharacterController().GetCurrentLeanAmount()) > 0.2)
					aiLeaning++;
				IEntity veh = DCO_VehicleCombat.GetVehicle(ent);
				if (veh)
				{
					aiVehicle++;
					vehicles.Insert(veh);
				}

				vector p = ent.GetOrigin();
				bool near = false;
				foreach (vector pp : playerPos)
				{
					if (vector.DistanceSqXZ(p, pp) <= 1000000)
					{
						near = true;
						break;
					}
				}
				if (near)
					aiNear++;
				else
					aiFar++;
			}
		}

		int groupsContact = 0;
		int idleNoWp = 0;
		foreach (AIGroup g : groups)
		{
			SCR_AIGroup sg = SCR_AIGroup.Cast(g);
			if (!sg)
				continue;
			DCO_GroupUtilityComponent gUtil = DCO_GroupUtilityComponent.Cast(sg.FindComponent(DCO_GroupUtilityComponent));
			if (gUtil && gUtil.HasState(DCO_EGroupState.IN_CONTACT))
				groupsContact++;
			array<AIWaypoint> wps = {};
			sg.GetWaypoints(wps);
			if (wps.IsEmpty() && !sg.IsSlave())
				idleNoWp++;
		}

		map<int, int> byTask = new map<int, int>();
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
		{
			foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
			{
				if (!cmd)
					continue;
				foreach (DCO_GroupUtilityComponent cg : cmd.GetOwnedGroups())
				{
					if (cg && !cg.IsPlayerGroup())
						byTask.Set(cg.GetTask(), byTask.Get(cg.GetTask()) + 1);
				}
			}
		}
		string tasks;
		foreach (int task, int n : byTask)
			tasks += string.Format(" %1=%2", typename.EnumToString(DCO_EGroupTask, task), n);

		if (aiTotal > s_iPeakAI)
			s_iPeakAI = aiTotal;
		if (pids.Count() > s_iPeakPlayers)
			s_iPeakPlayers = pids.Count();
		if (groupsContact > s_iPeakGroupsContact)
			s_iPeakGroupsContact = groupsContact;

		Write(DCO_VehicleActive.CensusLine());
		Write(DCO_ReinfBoard.CensusLine());
		Write(string.Format("CENSUS pop players=%1 ai_total=%2 ai_contact=%3 ai_vehicle=%4 ai_near_1km=%5 ai_far=%6 dco_locked=%7 ai_vehicles=%8 ai_leaning=%9",
			pids.Count(), aiTotal, aiContact, aiVehicle, aiNear, aiFar, aiLocked, vehicles.Count(), aiLeaning));
		Write(string.Format("CENSUS morale calm=%1 tense=%2 shaken=%3 panic=%4 break=%5 spirited=%6", mCalm, mTense, mShaken, mPanic, mBreak, mSpirited));
		foreach (string sk, DCO_CensusSide sd : sides)
		{
			Write(string.Format("CENSUS threat side=%1 n=%2 supp=%3 flyby=%4 endangered=%5 shots=%6 injury=%7 total=%8 states=%9",
				sk, sd.n, DCO_CensusSide.Avg(sd.supp, sd.n), DCO_CensusSide.Avg(sd.fly, sd.n), DCO_CensusSide.Avg(sd.end, sd.n), DCO_CensusSide.Avg(sd.shots, sd.n), DCO_CensusSide.Avg(sd.inj, sd.n), DCO_CensusSide.Avg(sd.total, sd.n),
				string.Format("safe:%1/vig:%2/alert:%3/threat:%4 stress=S0:%5/S1:%6/S2:%7", sd.tSafe, sd.tVig, sd.tAlert, sd.tThreat, sd.s0, sd.s1, sd.s2)));
			Write(string.Format("CENSUS morale_c side=%1 n=%2 supp=%3 endangered=%4 events=%5 relief=%6 condition=%7 boost=%8 states=%9",
				sk, sd.n, DCO_CensusSide.Avg(sd.mSupp, sd.n), DCO_CensusSide.Avg(sd.mEnd, sd.n), DCO_CensusSide.Avg(sd.mEv, sd.n), DCO_CensusSide.Avg(sd.mRel, sd.n), DCO_CensusSide.Avg(sd.mCond, sd.n), DCO_CensusSide.Avg(sd.mBoost, sd.n),
				string.Format("calm:%1/tense:%2/shaken:%3/panic:%4/break:%5/spirited:%6", sd.mCalm, sd.mTense, sd.mShaken, sd.mPanic, sd.mBreak, sd.mSpirited)));
		}
		Write(string.Format("CENSUS groups total=%1 contact=%2 idle_nowp=%3 commander:%4", groups.Count(), groupsContact, idleNoWp, tasks));
	}

	protected static string Rate(string source)
	{
		float sec = Math.Max(s_fTimer, 1);
		return (DCO_Perf.CensusCount(source) / sec).ToString(-1, 2);
	}

	protected static void WriteRates()
	{
		Write(string.Format("CENSUS rate/s shot_react=%1 hits=%2 calllater=%3 spawn_flare=%4 spawn_smoke=%5 spawn_smoke_arty=%6 spawn_illum=%7 rounds=%8",
			Rate("ev:shot"), Rate("ev:hit"), Rate("cl:shot"), Rate("sp:flare"), Rate("sp:smoke"), Rate("sp:smoke_arty"), Rate("sp:illum"), Rate("ev:round")));
		Write(string.Format("CENSUS gren/s try=%1 ok=%2 rej_friendly=%3 rej_far=%4 rej_close=%5 rej_arc=%6 rej_cooldown=%7 rej_chance=%8 rej_threat=%9",
			Rate("gren:try"), Rate("gren:ok"), Rate("gren:rej_friendly"), Rate("gren:rej_far"), Rate("gren:rej_close"), Rate("gren:rej_arc"), Rate("gren:rej_cooldown"), Rate("gren:rej_chance"), Rate("gren:rej_threat")));
		Write(string.Format("CENSUS gren2/s breach_ok=%1 drop_ok=%2 badfrag=%3 explosion=%4 exp_prone=%5 exp_displace=%6 exp_idf_report=%7 exp_grenade_cover=%8",
			Rate("gren:breach_ok"), Rate("gren:drop_ok"), Rate("gren:badfrag"), Rate("ev:explosion"), Rate("exp_prone"), Rate("exp_displace"), Rate("exp_idf_report"), Rate("exp_grenade_cover")));
		Write(string.Format("CENSUS gren3/s close_cover=%1 close_cluster=%2", Rate("gren:close_cover"), Rate("gren:close_cluster")));
		Write(string.Format("CENSUS drills/s bc_start=%1 amb_detect=%2 sop_sprung=%3 sop_detour=%4 rearm_start=%5 rearm_corpse=%6 rearm_station=%7 recover_ok=%8 recover_fail=%9",
			Rate("bc:start"), Rate("amb:detect"), Rate("sop:sprung"), Rate("sop:detour"), Rate("rearm:start"), Rate("rearm:corpse"), Rate("rearm:station"), Rate("recover:equip_ok"), Rate("recover:equip_fail")));
		Write(string.Format("CENSUS attack/s no_bypass_obj=%1 staging_return_fire=%2 reinforce=%3 abort_losses=%4 abort_no_progress=%5",
			Rate("sop:no_bypass_obj"), Rate("sop:staging_return_fire"), Rate("attack_reinforce"), Rate("attack_abort_reason:losses"), Rate("attack_abort_reason:no_progress")));
		Write(string.Format("CENSUS reattack/s watchdog=%1 stale_reset=%2 skip_recon_wait=%3 skip_recon_sent=%4 skip_no_tactic=%5 skip_overrun=%6 skip_cooldown=%7 standoff_esc=%8 standoff_withdraw=%9",
			Rate("attack_watchdog"), Rate("obj_reset_stale"), Rate("obj_skip:recon_wait"), Rate("obj_skip:recon_sent"), Rate("obj_skip:no_tactic"), Rate("obj_skip:overrun"), Rate("obj_skip:cooldown"),
			((1.0 * DCO_Perf.CensusCount("standoff_escalate:ratio") + DCO_Perf.CensusCount("standoff_escalate:est_drop") + DCO_Perf.CensusCount("standoff_escalate:quiet")
				+ DCO_Perf.CensusCount("standoff_escalate:timeout_assault")) / Math.Max(s_fTimer, 1)).ToString(-1, 2), Rate("standoff_withdraw")));
		Write(string.Format("CENSUS lean/s peek_sound=%1 peek_shoot=%2 peek_lost=%3 peek_sector=%4 cqb_pie=%5 cqb_hall=%6 cqb_door=%7 voice_try=%8 voice_heard=%9",
			Rate("lean:peek_sound"), Rate("lean:peek_shoot"), Rate("lean:peek_lost"), Rate("lean:peek_sector"), Rate("lean:cqb_pie"), Rate("lean:cqb_hall"), Rate("lean:cqb_door"), Rate("hear:voice_try"), Rate("hear:voice")));
		Write(string.Format("CENSUS navlink/s climb=%1 climb_no_move=%2 ladder=%3 ladder_no_move=%4 stand_retry=%5 stall=%6 stall_obstacle=%7 block_bt=%8 block_req=%9",
			Rate("navlink:climb_start"), Rate("navlink:climb_no_move"), Rate("navlink:ladder_start"), Rate("navlink:ladder_no_move"), Rate("navlink:stand_retry"), Rate("navlink:stall"), Rate("navlink:stall_at_obstacle"), Rate("navlink:block_bt_stance"), Rate("navlink:block_req_stance")));
		Write(string.Format("CENSUS reflex/s start=%1 switch=%2 attack_high=%3 fire_override=%4 ctrl_survival=%5 duck_out=%6 duck_in=%7 aim_keep=%8",
			Rate("reflex:start"), Rate("reflex:switch_target"), Rate("reflex:attack_high"), Rate("reflex:fire_override"), Rate("reflex:ctrl_survival"), Rate("reflex:duck_outdoor"), Rate("reflex:duck_indoor"), Rate("reflex:aim_keep_moving")));
		Write(string.Format("CENSUS coverfight/s peek=%1 hide=%2 hide_fire=%3 shift_out=%4 shift_in=%5 shift_fail=%6 pierced_out=%7 pierced_in=%8",
			Rate("cover:peek"), Rate("cover:peek_hide"), Rate("cover:peek_hide_under_fire"), Rate("cover:shift_outdoor"), Rate("cover:shift_indoor"), Rate("cover:shift_fail_indoor"), Rate("cover:pierced_outdoor"), Rate("cover:pierced_indoor")));
		Write(string.Format("CENSUS coverdiag/s cycle_start=%1 hidden_seen=%2 hide_not_applied=%3 spot_reset=%4 field_wall=%5 field_low=%6 field_none=%7",
			Rate("cover:peek_cycle_start"), Rate("cover:hidden_seen"), Rate("cover:hide_not_applied"), Rate("cover:peek_spot_reset"), Rate("cover:field_spot_1"), Rate("cover:field_spot_2"), Rate("cover:field_no_cover")));
		Write(string.Format("CENSUS flush/s grenade=%1 approach=%2 gren_rej=%3 gren_not_now=%4 flank=%5 flank_no_nav=%6 approach_no_nav=%7",
			Rate("flush:grenade"), Rate("flush:approach"), Rate("flush:gren_rejected"), Rate("flush:gren_not_now"), Rate("flush:flank"), Rate("flush:flank_no_nav"), Rate("flush:approach_no_nav")));
		Write(string.Format("CENSUS smoke/s tracked_hand=%1 tracked_gl=%2 cloud=%3 fire_block=%4 suppress=%5 aim_degraded=%6 watch=%7 mg_suppress=%8 exit=%9",
			Rate("smoke:tracked_hand"), Rate("smoke:tracked_gl"), Rate("smoke:cloud"), Rate("smoke:fire_block"), Rate("smoke:suppress"), Rate("smoke:aim_degraded"), Rate("smoke:watch"), Rate("smoke:mg_suppress"), Rate("smoke:exit")));
		Write(string.Format("CENSUS search/s start=%1 arrive=%2 corner=%3 head_on=%4 head_off=%5 spot=%6 reached=%7 timeout=%8 clear=%9",
			Rate("search:start"), Rate("search:arrive"), Rate("search:corner_look"), Rate("search:corner_head_on"), Rate("search:corner_head_off"), Rate("search:spot"), Rate("search:spot_reached"), Rate("search:spot_timeout"), Rate("search:clear")));
		Write(string.Format("CENSUS searchend/s found=%1 found_approach=%2 found_search=%3 cut=%4 no_spot=%5 override_move=%6 look_rejected=%7",
			Rate("search:found"), Rate("search:found_approach"), Rate("search:found_search"), Rate("search:cut"), Rate("search:clear_no_spot"), Rate("search:spot_override_move"), Rate("search:look_rejected")));
		Write(string.Format("CENSUS grenwin/s window_ok=%1 window_none=%2 cook=%3 cook_nofind=%4 rej_arc=%5", Rate("gren:window_ok"), Rate("gren:window_none"), Rate("gren:cook"), Rate("gren:cook_nofind"), Rate("gren:rej_arc")) + string.Format(" thrown=%1 cook_skip_flight=%2", Rate("gren:thrown_frag"), Rate("gren:cook_skip_flight")));
		Write(string.Format("CENSUS cqbaggr/s push_burst=%1 push_under_fire=%2 fallback_cas=%3 sprint=%4 breach_ok=%5 yield=%6 point_blank=%7 room_cleared=%8 entry_cas=%9", Rate("cqb:push_burst"), Rate("cqb:push_under_fire"), Rate("cqb:fallback_casualties"), Rate("cqb:move_sprint"), Rate("gren:breach_ok"), Rate("cqb:yield_1"), Rate("cqb:yield_point_blank"), Rate("cqb:room_cleared"), Rate("cqb:entry_casualty")) + string.Format(" breach_hold_cover=%1 charge=%2 charge_roll=%3 approach_reorder=%4 stack_partial=%5", Rate("cqb:breach_hold_cover"), Rate("cqb:charge"), Rate("cqb:charge_roll"), Rate("cqb:approach_reorder"), Rate("cqb:stack_partial")));
		Write(string.Format("CENSUS lostgap/s idle_long=%1 idle_short=%2 blind_switch=%3 blind_back=%4", Rate("lostgap:idle_long"), Rate("lostgap:idle_short"), Rate("lostgap:blind_switch"), Rate("lostgap:blind_back")) + string.Format(" inv_start=%1 slow_start=%2", Rate("lostgap:investigate_start"), Rate("lostgap:slow_start")));
		Write(string.Format("CENSUS navface/s face_start=%1 face=%2 look_blocked=%3 block_lean=%4 block_idle=%5",
			Rate("navlink:face_start"), Rate("navlink:face"), Rate("navlink:look_blocked"), Rate("navlink:block_lean"), Rate("navlink:block_idle_stance")));
		Write(string.Format("CENSUS veh/s at_active=%1 push_no_at=%2 pullback_at=%3 wait_at=%4 wait_close=%5 wait_lead=%6",
			Rate("veh:at_active"), Rate("veh:push_no_at"), Rate("veh:pullback_at"), Rate("veh:wait_at_active"), Rate("veh:wait_close_terrain"), Rate("veh:wait_lead_limit")));
		Write(string.Format("CENSUS duel/s hulldown=%1 scoot=%2 flank=%3 withdraw=%4 request=%5",
			Rate("veh:duel_hulldown"), Rate("veh:duel_scoot"), Rate("veh:duel_flank"), Rate("veh:duel_withdraw"), Rate("veh:duel_request")));
		Write(string.Format("CENSUS gunner/s tgt_at=%1 tgt_armor=%2 tgt_mg=%3 tgt_cluster=%4 tgt_single=%5 ammo_switch=%6 cover_move=%7",
			Rate("veh:tgt_at"), Rate("veh:tgt_armor"), Rate("veh:tgt_mg"), Rate("veh:tgt_cluster"), Rate("veh:tgt_single"), Rate("veh:ammo_switch"), Rate("veh:cover_squad_move")));
		Write(string.Format("CENSUS antiarmor/s at_flank=%1 at_pair=%2 inf_cover=%3 inf_request=%4",
			Rate("ev:at_flank"), Rate("ev:at_pair"), Rate("ev:inf_armor_cover"), Rate("ev:inf_armor_request")));
		Write(string.Format("CENSUS mounted/s dismount_motor=%1 dismount_mech=%2 mech_follow=%3 pickup=%4 remount=%5 crew_replace=%6 vehicle_lost=%7 pool_kept=%8 split=%9",
			Rate("veh:dismount_motor"), Rate("veh:dismount_mech"), Rate("veh:mech_follow"), Rate("veh:pickup"), Rate("veh:remount"), Rate("veh:crew_replace"), Rate("veh:vehicle_lost"), Rate("veh:pool_kept"), Rate("veh:split_vehicle")));
		Write(string.Format("CENSUS pair/s form=%1 follow=%2 inf_lead_urban=%3 marked=%4 idle_support=%5 break_dist=%6 break_idle=%7 break_diverge=%8 break_cmd=%9",
			Rate("pair:form"), Rate("pair:armor_follow_inf"), Rate("pair:inf_lead_urban"), Rate("pair:marked_target_fire"), Rate("pair:idle_armor_support"), Rate("pair:break:lost_infantry"), Rate("pair:break:idle"), Rate("pair:break:diverge"), Rate("pair:break:commander")));
		Write(string.Format("CENSUS fire/s sup_lk_start=%1 sup_lk_end=%2 lost_contact=%3 assault_fire=%4 react_delay=%5 react_behind=%6 startle=%7",
			Rate("sup_lk_start"), Rate("sup_lk_end"), Rate("lost_contact_report"), Rate("assault_fire_move"), Rate("react_delay"), Rate("react_behind"), Rate("startle")));
		Write(string.Format("CENSUS ce/s hold=%1 forced_nearmiss=%2 forced_objective=%3 forced_route=%4 group_free=%5 resume=%6",
			Rate("ce_hold"), Rate("ce_forced_nearmiss"), Rate("ce_forced_objective"), Rate("ce_forced_route"), Rate("ce_group_free"), Rate("ce_resume")));
		Write(string.Format("CENSUS armor_fear/s scaled=%1 unscaled=%2", Rate("morale:armor_fear_scaled"), Rate("morale:armor_fear")));
		Write(string.Format("CENSUS threat_in/s crossfire=%1 unseen_shooter=%2 contagion=%3 armor_fear=%4 fatigue=%5 hysteresis_hold=%6 explosion_close=%7 veteran=%8",
			Rate("threat:crossfire"), Rate("morale:unseen_shooter"), Rate("morale:contagion"), Rate("morale:armor_fear"), Rate("morale:fatigue"), Rate("threat:hysteresis_hold"), Rate("threat:explosion_close"), Rate("morale:veteran")));
		Write(string.Format("CENSUS morale_in/s friend_killed=%1 leader_killed=%2 enemy_killed=%3 enemy_retreat=%4 outnumbered=%5 isolated=%6 friendly_support=%7",
			Rate("morale:friend_killed"), Rate("morale:leader_killed"), Rate("morale:enemy_killed"), Rate("morale:enemy_retreat"), Rate("morale:outnumbered"), Rate("morale:isolated"), Rate("morale:friendly_support")));
		Write(string.Format("CENSUS posture/s aggresive=%1 balance=%2 defensive=%3 evasive=%4 delay=%5 ambush=%6 sneak=%7",
			Rate("posture_change:AGGRESIVE"), Rate("posture_change:BALANCE"), Rate("posture_change:DEFENSIVE"), Rate("posture_change:EVASIVE"), Rate("posture_change:DELAY"), Rate("posture_change:AMBUSH"), Rate("posture_change:SNEAK")));
		Write(string.Format("CENSUS aggr/s push=%1", Rate("aggr_push")));
		Write(string.Format("CENSUS assault/s start=%1 bound=%2 overwatch=%3 swap=%4 arrived=%5 timeout=%6 blocked_pinned=%7 assault=%8 consolidate=%9",
			Rate("asl:start"), Rate("asl:bound"), Rate("asl:overwatch"), Rate("asl:swap"), Rate("asl:bound_arrived"), Rate("asl:bound_timeout"), Rate("asl:blocked_pinned"), Rate("asl:ASSAULT"), Rate("asl:CONSOLIDATE")));
		Write(string.Format("CENSUS tacrole/s start=%1 staging=%2 executing=%3 holding=%4 withdraw=%5 signal=%6 contact_block=%7 fireline=%8 fix_fire=%9",
			Rate("tacrole:start"), Rate("tacrole:STAGING"), Rate("tacrole:EXECUTING"), Rate("tacrole:HOLDING"), Rate("tacrole:WITHDRAW"), Rate("tacrole:signal"), Rate("tacrole:contact_block"), Rate("tacrole:fireline"), Rate("tacrole:fix_fire"))
			+ string.Format(" fix_shift=%1 fix_lift=%2", Rate("tacrole:fix_shift"), Rate("tacrole:fix_lift")));
		Write(string.Format("CENSUS prone/s crawl_fwd=%1 roll_side=%2 crawl_back=%3 back_ok=%4 back_fail=%5 to_crouch_fwd=%6 to_crouch_side=%7 to_crouch_back=%8",
			Rate("prone_crawl_fwd"), Rate("prone_roll_side"), Rate("prone_crawl_back"), Rate("prone_back_facing_ok"), Rate("prone_back_facing_fail"), Rate("prone_to_crouch:far_fwd"), Rate("prone_to_crouch:side"), Rate("prone_to_crouch:back")));
		Write(string.Format("CENSUS pin/s start=%1 hold_s=%2 break=%3 recover=%4 burst=%5 block_move=%6 panic_crawl=%7 group=%8 group_end=%9",
			Rate("pin_start"), Rate("pin_hold"), Rate("pin_break"), Rate("pin_recover"), Rate("pin_burst"), Rate("pin_block_move"), Rate("pin_panic_crawl"), Rate("pin_group"), Rate("pin_group_end")));
		Write(string.Format("CENSUS pin_group/s smoke=%1 flank=%2 flank_blocked=%3 support=%4",
			Rate("pin_group_smoke"), Rate("pin_group_flank"), Rate("pin_group_flank_blocked"), Rate("pin_group_support")));
		Write(string.Format("CENSUS firetree/s hold_look=%1 hold_break=%2 hold_assault_move=%3 rpg_unseen_fire=%4 block_close=%5 block_far=%6 block_cd=%7 block_ammo=%8 block_soft=%9",
			Rate("hold_fire_look"), Rate("hold_fire_break"), Rate("hold_fire_assault_move"), Rate("rpg_unseen_fire"), Rate("rpg_unseen_block:close"), Rate("rpg_unseen_block:far"), Rate("rpg_unseen_block:cooldown"), Rate("rpg_unseen_block:ammo"), Rate("rpg_unseen_block:soft")));
		Write(string.Format("CENSUS posture_ops/s ft_cover=%1 bal_bound=%2 hold_move=%3 overrun=%4 evasive=%5 evasive_quiet=%6 delay_hold=%7 delay_bound=%8 bound_done=%9",
			Rate("ft_cover"), Rate("bal_bound"), Rate("pos_hold_move"), Rate("pos:overrun"), Rate("pos:evasive_withdraw"), Rate("pos:evasive_quiet"), Rate("pos:delay_hold"), Rate("pos:delay_bound"), Rate("bc:bound_done")));
		Write(string.Format("CENSUS posture_ops/s sneak_spotted=%1 ambush_set=%2 ambush_sprung=%3 ambush_assault=%4 ambush_withdraw=%5",
			Rate("pos:sneak_spotted"), Rate("pos:ambush_set"), Rate("pos:ambush_sprung"), Rate("pos:ambush_assault"), Rate("pos:ambush_withdraw")));
		Write(string.Format("CENSUS voice/s heard=%1 bearing_only=%2 ambush=%3 sprung=%4 flank=%5 recon_fire=%6 prone=%7 garrison_alert=%8 report=%9",
			Rate("hear:voice"), Rate("voice:bearing_only"), Rate("voice:ambush"), Rate("voice:ambush_sprung"), Rate("voice:flank"), Rate("voice:recon_fire"), Rate("voice:cautious_prone"), Rate("voice:garrison_alert"), Rate("voice:report")));
		Write(string.Format("CENSUS rpc/s radio=%1 radiokey=%2 contact=%3 overlay=%4 marks=%5 orders=%6 float_per_s=%7",
			Rate("rpc:radio"), Rate("rpc:radiokey"), Rate("rpc:contact"), Rate("rpc:overlay"), Rate("rpc:marks"), Rate("rpc:orders"), Rate("rpc:floats")));
	}

	protected static void WriteQueues()
	{
		int arty = 0;
		int logi = 0;
		AICommander_ManagerComponent mgr = AICommander_ManagerComponent.GetInstance();
		if (mgr)
		{
			foreach (AICommander_BaseComponent cmd : mgr.m_aCommander)
			{
				if (!cmd)
					continue;
				if (cmd.GetArtySupport())
					arty += cmd.GetArtySupport().GetQueueCount();
				if (cmd.GetLogistics())
					logi += cmd.GetLogistics().GetJobs().Count();
			}
		}

		int garrison = 0;
		DCO_GarrisonRegistry reg = DCO_GarrisonRegistry.GetInstance();
		if (reg)
			garrison = reg.GetQueueCount();

		int spawner = 0;
		foreach (DCO_CommanderSpawnerComponent sp : DCO_CommanderSpawnerComponent.GetAll())
		{
			if (sp && sp.IsPending())
				spawner++;
		}

		int memKB = System.MemoryAllocationKB();
		Write(string.Format("CENSUS queue contact_pending=%1 garrison_queue=%2 artillery_queue=%3 logistics_jobs=%4 spawner_queue=%5 mem_kb=%6 alloc_count=%7 mem_delta_kb=%8",
			DCO_ContactSharing.PendingCount(), garrison, arty, logi, spawner, memKB, System.MemoryAllocationCount(), memKB - s_iPrevMemKB));
		s_iPrevMemKB = memKB;
	}

	protected static void WriteSummary(string reason)
	{
		float avg = 0;
		if (s_iShareSamples > 0)
			avg = s_fShareSum / s_iShareSamples;

		array<string> names = {};
		array<int> counts = {};
		if (s_mSpikeTops)
		{
			foreach (string name, int n : s_mSpikeTops)
			{
				int at = 0;
				while (at < counts.Count() && counts[at] >= n)
					at++;
				names.InsertAt(name, at);
				counts.InsertAt(n, at);
			}
		}
		string tops;
		for (int i = 0; i < names.Count() && i < 5; i++)
			tops += string.Format(" %1=%2", names[i], counts[i]);

		Write(string.Format("CENSUS summary reason=%1 dco_share_avg=%2 dco_share_peak=%3 peak_ai=%4 peak_players=%5 peak_groups_contact=%6 mem_kb=%7->%8 spike_tops:%9",
			reason, avg.ToString(-1, 1), s_fSharePeak.ToString(-1, 1), s_iPeakAI, s_iPeakPlayers, s_iPeakGroupsContact, s_iStartMemKB, System.MemoryAllocationKB(), tops));
	}

	protected static void Write(string line)
	{
		DCO_Log.Census(line);
	}
}
