class DCO_VehDbgRec
{
	float m_fSeen_ms = -1;
	string m_sMoveSrc = "-";
	float m_fMoveAt_ms = -1;
	vector m_vMoveTo;
	float m_fMoveD = -1;
	string m_sDrop = "-";
	float m_fDropAt_ms = -1;
	string m_sExit;
	float m_fExitAt_ms = -1;
}

class DCO_VehDbg
{
	protected static int s_iMode;
	protected static bool s_bDrawn;
	protected static ref map<IEntity, ref DCO_VehDbgRec> s_mRec;
	protected static map<IEntity, ref DCO_VehDbgRec> GetRecs()
	{
		if (!s_mRec)
			s_mRec = new map<IEntity, ref DCO_VehDbgRec>();
		return s_mRec;
	}
	protected static ref array<ref Shape> s_aShapes;
	protected static array<ref Shape> GetShapes()
	{
		if (!s_aShapes)
			s_aShapes = {};
		return s_aShapes;
	}
	protected static ref array<ref DebugTextWorldSpace> s_aTexts;
	protected static array<ref DebugTextWorldSpace> GetTexts()
	{
		if (!s_aTexts)
			s_aTexts = {};
		return s_aTexts;
	}

	static void Start()
	{
		if (!GetGame().InPlayMode() || System.IsConsoleApp())
			return;
		GetGame().GetCallqueue().Remove(Tick);
		GetGame().GetCallqueue().CallLater(Tick, 500, true);
	}

	static bool IsOn()
	{
		return s_iMode > 0;
	}

	protected static DCO_VehDbgRec Get(IEntity veh)
	{
		DCO_VehDbgRec r = GetRecs().Get(veh);
		if (r)
			return r;
		if (GetRecs().Count() > 128)
			Prune(GetGame().GetWorld().GetWorldTime());
		r = new DCO_VehDbgRec();
		GetRecs().Set(veh, r);
		return r;
	}

	static void Touch(IEntity veh)
	{
		if (s_iMode <= 0 || !veh)
			return;
		Get(veh).m_fSeen_ms = GetGame().GetWorld().GetWorldTime();
	}

	static void NoteMove(IEntity veh, string src, vector to, float dist)
	{
		if (s_iMode <= 0 || !veh)
			return;
		DCO_VehDbgRec r = Get(veh);
		r.m_sMoveSrc = src;
		r.m_fMoveAt_ms = GetGame().GetWorld().GetWorldTime();
		r.m_vMoveTo = to;
		r.m_fMoveD = dist;
	}

	static void NoteDrop(IEntity veh, string gate)
	{
		if (s_iMode <= 0 || !veh)
			return;
		DCO_VehDbgRec r = Get(veh);
		r.m_sDrop = gate;
		r.m_fDropAt_ms = GetGame().GetWorld().GetWorldTime();
	}

	static void NoteExit(IEntity veh, string text)
	{
		if (s_iMode <= 0 || !veh)
			return;
		DCO_VehDbgRec r = Get(veh);
		r.m_sExit = text;
		r.m_fExitAt_ms = GetGame().GetWorld().GetWorldTime();
	}

	static void ClearDraw()
	{
		GetShapes().Clear();
		GetTexts().Clear();
		s_bDrawn = false;
	}

	static void Tick()
	{
		BaseWorld world = GetGame().GetWorld();
		DCO_GlobalAIComponent gai = DCO_GlobalAIComponent.GetInstance();
		if (!world || !gai || !Replication.IsServer())
		{
			s_iMode = 0;
			if (s_bDrawn)
				ClearDraw();
			return;
		}
		s_iMode = Math.Round(gai.DCO_TuneF("va.debug", 0));
		bool show = s_iMode > 0 && (gai.DCO_TuneF("va.debug_always", 0) > 0 || DCO_DebugDraw.IsLocalPlayerInGM());
		if (!show)
		{
			if (s_bDrawn)
				ClearDraw();
			return;
		}
		Draw(world);
	}

	protected static void Draw(BaseWorld world)
	{
		GetShapes().Clear();
		GetTexts().Clear();
		s_bDrawn = true;
		float now = world.GetWorldTime();
		vector cam[4];
		world.GetCurrentCamera(cam);
		array<IEntity> vehs = {};
		array<float> dists = {};
		foreach (IEntity veh, DCO_VehDbgRec rec : GetRecs())
		{
			if (!veh || !rec || now - rec.m_fSeen_ms > 3000.0)
				continue;
			float d = vector.Distance(cam[3], veh.GetOrigin());
			if (d > 1500.0)
				continue;
			int at = 0;
			while (at < dists.Count() && dists[at] < d)
				at++;
			vehs.InsertAt(veh, at);
			dists.InsertAt(d, at);
		}
		int shown = Math.Min(vehs.Count(), 24);
		for (int i = 0; i < shown; i++)
			DrawVehicle(vehs[i], GetRecs().Get(vehs[i]), dists[i], now);
	}

	protected static void DrawVehicle(IEntity veh, DCO_VehDbgRec rec, float camD, float now)
	{
		DCO_VehBrainRec br = DCO_VehBrain.Find(veh);
		string state = "?";
		string why = "-";
		if (br)
		{
			state = br.m_sState;
			why = br.m_sWhy;
		}
		int color = StateColor(state);
		int cls = DCO_VehicleActive.VehClass(veh);

		string l1 = string.Format("%1 %2 · %3", DCO_Log.IdVeh(veh), DCO_VehicleActive.ClassName(cls), state);
		if (why != "-")
			l1 += " · " + why;
		if (br)
			l1 += string.Format(" · %1s", Math.Round((now - br.m_fStateAt_ms) / 1000.0));

		string l2 = "no target";
		if (br && br.m_fTgtD >= 0)
		{
			string kind = "inf";
			if (br.m_bTgtVeh)
				kind = "veh";
			l2 = string.Format("tgt %1m %2 seen %3s", Math.Round(br.m_fTgtD), kind, Math.Round(br.m_fTgtSeen_s));
		}
		if (br)
		{
			l2 += string.Format(" · AT %1 · %2", YesNo(br.m_bAT), br.m_sPosture);
			if (br.m_bRelentless)
				l2 += " · RELENTLESS";
		}

		string l3 = string.Format("moves/min %1 offered %2 applied", DCO_VehMove.Rate(veh), DCO_VehMove.AppliedRate(veh));
		if (rec.m_fMoveAt_ms >= 0)
			l3 += string.Format(" · last %1 %2m %3s ago", rec.m_sMoveSrc, Math.Round(rec.m_fMoveD), Math.Round((now - rec.m_fMoveAt_ms) / 1000.0));
		if (rec.m_fDropAt_ms >= 0 && now - rec.m_fDropAt_ms < 10000.0)
			l3 += " · drop " + rec.m_sDrop;
		if (br && br.m_fHeldAt_ms >= 0 && now - br.m_fHeldAt_ms < 10000.0)
			l3 += " · held " + br.m_sHeld;

		array<string> lines = {};
		array<int> colors = {};
		lines.Insert(l1);
		colors.Insert(color);
		lines.Insert(l2);
		colors.Insert(0xFFDDDDDD);
		lines.Insert(l3);
		colors.Insert(0xFFDDDDDD);
		if (s_iMode >= 2)
		{
			lines.Insert(DCO_VehExit.Damage(veh) + " · crew " + DCO_VehicleActive.CrewAlive(veh).ToString());
			colors.Insert(0xFFBBBBBB);
		}
		if (rec.m_fExitAt_ms >= 0 && now - rec.m_fExitAt_ms < 10000.0)
		{
			lines.Insert("EXIT " + rec.m_sExit);
			colors.Insert(0xFFFF5544);
		}

		vector mins, maxs;
		veh.GetBounds(mins, maxs);
		float step = Math.Max(0.6, camD * 0.021);
		vector top = veh.GetOrigin() + Vector(0, maxs[1] + 1.0, 0);
		int n = lines.Count();
		for (int i = 0; i < n; i++)
			Text(top + Vector(0, (n - 1 - i) * step, 0), lines[i], 13.0, colors[i]);

		if (s_iMode < 2)
			return;
		vector lift = Vector(0, 1.0, 0);
		if (state == "MOVE" && rec.m_vMoveTo != vector.Zero)
		{
			Line(0xFFFFCC00, veh.GetOrigin() + lift, rec.m_vMoveTo + lift);
			Ball(0xFFFFCC00, rec.m_vMoveTo + lift, 0.6);
		}
		DCO_VehActState va = DCO_VehicleActive.Find(veh);
		if (va && va.m_vLastFire != vector.Zero)
			Ball(0xFF33CC66, va.m_vLastFire + lift, 0.8);
		if (br && br.m_fTgtD >= 0)
			Ball(0xFFFF8833, br.m_vTgt + lift, 0.5);
		IEntity atShooter;
		vector atPos;
		if (DCO_VehicleCombat.GetATThreat(veh, atShooter, atPos))
			Line(0xFFFF4444, veh.GetOrigin() + lift, atPos + lift);
	}

	protected static int StateColor(string state)
	{
		switch (state)
		{
			case "ENGAGE": return 0xFF33CC66;
			case "OVERWATCH": return 0xFF99DDAA;
			case "MOVE": return 0xFFFFCC00;
			case "TRAVEL": return 0xFF33AAFF;
			case "HIDE": return 0xFFFF8833;
			case "WITHDRAW": return 0xFFFF8833;
			case "IMMOBILE": return 0xFFFF4444;
			case "EMPTY": return 0xFF888888;
		}
		return 0xFFBBBBBB;
	}

	protected static string YesNo(bool b)
	{
		if (b)
			return "yes";
		return "no";
	}

	protected static void Line(int color, vector a, vector b)
	{
		vector seg[2];
		seg[0] = a;
		seg[1] = b;
		GetShapes().Insert(Shape.CreateLines(color, ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP, seg, 2));
	}

	protected static void Ball(int color, vector p, float r)
	{
		GetShapes().Insert(Shape.CreateSphere(color, ShapeFlags.NOZBUFFER | ShapeFlags.TRANSP, p, r));
	}

	protected static void Text(vector p, string text, float size, int color)
	{
		DebugTextWorldSpace t = DCO_DebugDraw.SpawnText(p, text, size, color);
		if (t)
			GetTexts().Insert(t);
	}

	protected static void Prune(float now)
	{
		array<IEntity> stale = {};
		foreach (IEntity v, DCO_VehDbgRec r : GetRecs())
		{
			if (!v || !r || now - r.m_fSeen_ms > 60000.0)
				stale.Insert(v);
		}
		foreach (IEntity sv : stale)
			GetRecs().Remove(sv);
	}
}

modded class DCO_GlobalAIComponent
{
	override void OnPostInit(IEntity owner)
	{
		super.OnPostInit(owner);
		DCO_VehDbg.Start();
	}
}
