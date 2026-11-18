class DCO_INavHop
{
	int m_iA;
	int m_iB;
	int m_iDoor = -1;
	vector m_vA;
	vector m_vB;
}

class DCO_INavCQBPlan
{
	DCO_INavGraph m_G;
	ref array<ref DCO_INavHop> m_aHops = {};
	ref array<int> m_aGraphRoom = {};
	ref array<int> m_aOrder = {};
	ref DCO_INavHop m_Entry;
	ref DCO_INavHop m_Hop;
	ref DCO_INavHop m_Entry2;
	ref map<int, float> m_mDoorTry = new map<int, float>();
	ref map<SCR_ChimeraAIAgent, float> m_mPassAt = new map<SCR_ChimeraAIAgent, float>();
	ref map<SCR_ChimeraAIAgent, int> m_mPassDoor = new map<SCR_ChimeraAIAgent, int>();
	ref array<float> m_aBelief;
	int m_iBeliefFloor = -1;
	int m_iUnreached;
	int m_iUnreachedFloors;
	string m_sGrp;

	static DCO_INavCQBPlan Make(IEntity building, vector from, array<vector> tried, string grp, out string why)
	{
		why = "wait";
		DCO_INavGraph g = DCO_IndoorNav.Get(building, true);
		if (!g)
			return null;
		if (g.m_aSamples.Count() < 8 || g.m_aRooms.IsEmpty())
		{
			why = "empty";
			return null;
		}
		DCO_INavCQBPlan p = new DCO_INavCQBPlan();
		p.m_G = g;
		p.m_sGrp = grp;
		p.BuildHops();
		if (!p.PickEntry(from, tried))
		{
			why = "no_outside_door";
			return null;
		}
		p.Order();
		if (p.m_aOrder.IsEmpty())
		{
			why = "no_rooms";
			return null;
		}
		p.CountUnreached();
		why = "-";
		return p;
	}

	protected void CountUnreached()
	{
		array<int> reachedFloors = {};
		foreach (int gid : m_aOrder)
		{
			int rf = m_G.m_aRooms[gid].m_iFloor;
			if (!reachedFloors.Contains(rf))
				reachedFloors.Insert(rf);
		}
		array<int> lostFloors = {};
		foreach (DCO_INavRoom r : m_G.m_aRooms)
		{
			if (!Clearable(r.m_iId) || r.m_aSamples.Count() < 6 || m_aOrder.Contains(r.m_iId))
				continue;
			m_iUnreached++;
			if (!reachedFloors.Contains(r.m_iFloor) && !lostFloors.Contains(r.m_iFloor))
				lostFloors.Insert(r.m_iFloor);
		}
		m_iUnreachedFloors = lostFloors.Count();
		if (m_iUnreachedFloors > 0)
			DCO_Perf.Count("cqb:plan_unreached_floor");
	}

	protected static vector Flat(vector v)
	{
		v[1] = 0;
		if (v.LengthSq() < 0.0001)
			return vector.Zero;
		return v.Normalized();
	}

	protected void BuildHops()
	{
		foreach (int di, DCO_INavDoor d : m_G.m_aDoors)
		{
			if (d.m_iRoomA == d.m_iRoomB)
				continue;
			DCO_INavHop h = new DCO_INavHop();
			h.m_iA = d.m_iRoomA;
			h.m_iB = d.m_iRoomB;
			h.m_iDoor = di;
			h.m_vA = d.m_vL - d.m_vN * 0.8;
			h.m_vB = d.m_vL + d.m_vN * 0.8;
			foreach (DCO_INavPoint p : m_G.m_aPoints)
			{
				if (p.m_iType != 1 || p.m_iDoor != di)
					continue;
				if (vector.Dot(p.m_vL - d.m_vL, d.m_vN) < 0)
					h.m_vA = p.m_vL;
				else
					h.m_vB = p.m_vL;
			}
			if (h.m_iA < 0 && h.m_iB < 0)
				continue;
			m_aHops.Insert(h);
		}
		foreach (int li, int kind : m_G.m_aLinkKind)
		{
			if (kind != 2)
				continue;
			DCO_INavSample sa = m_G.m_aSamples[m_G.m_aLinkA[li]];
			DCO_INavSample sb = m_G.m_aSamples[m_G.m_aLinkB[li]];
			if (sa.m_iRoom == sb.m_iRoom || sa.m_iRoom < 0 || sb.m_iRoom < 0)
				continue;
			bool dup = false;
			foreach (DCO_INavHop eh : m_aHops)
			{
				if ((eh.m_iA == sa.m_iRoom && eh.m_iB == sb.m_iRoom) || (eh.m_iA == sb.m_iRoom && eh.m_iB == sa.m_iRoom))
				{
					dup = true;
					break;
				}
			}
			if (dup)
				continue;
			DCO_INavHop sh = new DCO_INavHop();
			sh.m_iA = sa.m_iRoom;
			sh.m_iB = sb.m_iRoom;
			sh.m_vA = sa.m_vL;
			sh.m_vB = sb.m_vL;
			m_aHops.Insert(sh);
		}
	}

	protected DCO_INavHop Oriented(DCO_INavHop h, int from)
	{
		if (h.m_iA == from)
			return h;
		DCO_INavHop r = new DCO_INavHop();
		r.m_iA = h.m_iB;
		r.m_iB = h.m_iA;
		r.m_iDoor = h.m_iDoor;
		r.m_vA = h.m_vB;
		r.m_vB = h.m_vA;
		return r;
	}

	protected bool Clearable(int room)
	{
		if (room < 0 || m_G.m_aRooms[room].m_bOutside)
			return false;
		int kind = m_G.m_aRooms[room].m_iKind;
		return kind == 0 || kind == 1 || kind == 3;
	}

	protected bool IsStairHop(DCO_INavHop h)
	{
		return h.m_iDoor < 0 || (h.m_iA >= 0 && m_G.m_aRooms[h.m_iA].m_iKind == 2) || (h.m_iB >= 0 && m_G.m_aRooms[h.m_iB].m_iKind == 2);
	}

	protected bool PickEntry(vector from, array<vector> tried)
	{
		float best = float.MAX;
		foreach (DCO_INavHop h : m_aHops)
		{
			if (h.m_iA >= 0 && h.m_iB >= 0)
				continue;
			DCO_INavHop o = Oriented(h, -1);
			if (!Clearable(o.m_iB) && (o.m_iB < 0 || m_G.m_aRooms[o.m_iB].m_iKind != 2))
				continue;
			vector dw = m_G.W(o.m_vA);
			if (o.m_iDoor >= 0)
				dw = m_G.W(m_G.m_aDoors[o.m_iDoor].m_vL);
			bool skip = false;
			if (tried)
			{
				foreach (vector td : tried)
				{
					if (vector.DistanceXZ(td, dw) < 2.0)
						skip = true;
				}
			}
			if (skip)
				continue;
			float d = vector.DistanceXZ(m_G.W(o.m_vA), from);
			if (d < best)
			{
				best = d;
				m_Entry = o;
			}
		}
		return m_Entry != null;
	}

	protected void Order()
	{
		array<int> seen = {m_Entry.m_iB};
		array<int> primary = {m_Entry.m_iB};
		array<int> secondary = {};
		while (!primary.IsEmpty() || !secondary.IsEmpty())
		{
			int u;
			if (!primary.IsEmpty())
			{
				u = primary[0];
				primary.Remove(0);
			}
			else
			{
				u = secondary[0];
				secondary.Remove(0);
			}
			if (Clearable(u))
				m_aOrder.Insert(u);
			foreach (DCO_INavHop nh : m_aHops)
			{
				int v = -2;
				if (nh.m_iA == u)
					v = nh.m_iB;
				else if (nh.m_iB == u)
					v = nh.m_iA;
				if (v < -1 || seen.Contains(v))
					continue;
				seen.Insert(v);
				if (v == -1 || IsStairHop(nh))
					secondary.Insert(v);
				else
					primary.Insert(v);
			}
		}
	}

	protected vector RoomCenter(DCO_INavRoom r)
	{
		if (r.m_iCenter >= 0)
			return m_G.W(m_G.m_aSamples[r.m_iCenter].m_vL);
		vector c = vector.Zero;
		foreach (int si : r.m_aSamples)
			c = c + m_G.m_aSamples[si].m_vL;
		if (!r.m_aSamples.IsEmpty())
			c = c * (1.0 / r.m_aSamples.Count());
		return m_G.W(c);
	}

	void FillRooms(array<ref DCO_CQBRoom> rooms)
	{
		rooms.Clear();
		m_aGraphRoom.Clear();
		int stairs = 0;
		int slotsN = 0;
		foreach (DCO_INavRoom gr : m_G.m_aRooms)
		{
			if (gr.m_iKind == 2)
				stairs++;
		}
		string order;
		foreach (int gid : m_aOrder)
		{
			DCO_INavRoom r = m_G.m_aRooms[gid];
			DCO_CQBRoom cr = new DCO_CQBRoom();
			cr.m_vCenter = RoomCenter(r);
			cr.m_iFloor = r.m_iFloor;
			cr.m_iIdx = rooms.Count();
			array<DCO_INavPoint> pts = {};
			foreach (DCO_INavPoint p : m_G.m_aPoints)
			{
				if (p.m_iType != 3 || p.m_iRoom != gid)
					continue;
				int at = 0;
				while (at < pts.Count() && pts[at].m_iOrder <= p.m_iOrder)
					at++;
				pts.InsertAt(p, at);
			}
			foreach (DCO_INavPoint sp : pts)
				cr.m_aSlots.Insert(m_G.W(sp.m_vL));
			if (cr.m_aSlots.IsEmpty())
				cr.m_aSlots.Insert(cr.m_vCenter);
			slotsN += cr.m_aSlots.Count();
			rooms.Insert(cr);
			m_aGraphRoom.Insert(gid);
			if (!order.IsEmpty())
				order += ",";
			order += gid.ToString() + "f" + r.m_iFloor.ToString();
		}
		DCO_Perf.Count("cqb:plan_graph");
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_plan_graph grp=%1 prefab=%2 rooms=%3 floors=%4 stairs=%5 doors=%6 hops=%7 entry_door=%8 slots=%9", m_sGrp, DCO_Util.PrefabShort(m_G.m_Building), rooms.Count(), m_G.m_aFloors.Count(), stairs, m_G.m_aDoors.Count(), m_aHops.Count(), m_Entry.m_iDoor, slotsN) + " order=" + order + string.Format(" unreached_rooms=%1 unreached_floors=%2", m_iUnreached, m_iUnreachedFloors));
	}

	bool SecondEntry(vector from, out vector door, out vector outDir, out vector sa, out vector sb)
	{
		m_Entry2 = null;
		float best = float.MAX;
		vector e = m_G.W(m_Entry.m_vA);
		foreach (DCO_INavHop h : m_aHops)
		{
			if (h.m_iA != -1 && h.m_iB != -1)
				continue;
			DCO_INavHop o = Oriented(h, -1);
			if (o.m_iDoor < 0 || o.m_iB == m_Entry.m_iB || !Clearable(o.m_iB))
				continue;
			vector a = m_G.W(o.m_vA);
			if (vector.DistanceXZ(a, e) < 5.0)
				continue;
			float d = vector.DistanceXZ(a, from);
			if (d < best)
			{
				best = d;
				m_Entry2 = o;
			}
		}
		if (!m_Entry2)
			return false;
		door = m_G.W(m_Entry2.m_vA);
		outDir = Flat(door - m_G.W(m_Entry2.m_vB));
		StackSpots(m_Entry2, sa, sb);
		return outDir != vector.Zero;
	}

	int SecondRoom()
	{
		if (!m_Entry2)
			return -1;
		foreach (int ci, int gid : m_aGraphRoom)
		{
			if (gid == m_Entry2.m_iB)
				return ci;
		}
		return -1;
	}

	int Exits(notnull array<vector> pos, notnull array<vector> outDirs)
	{
		vector e = m_G.W(m_Entry.m_vA);
		foreach (DCO_INavHop h : m_aHops)
		{
			if (h.m_iDoor < 0 || (h.m_iA != -1 && h.m_iB != -1))
				continue;
			DCO_INavHop o = Oriented(h, -1);
			vector a = m_G.W(o.m_vA);
			if (vector.DistanceXZ(a, e) < 4.0)
				continue;
			vector d = Flat(a - m_G.W(o.m_vB));
			if (d == vector.Zero)
				continue;
			pos.Insert(a);
			outDirs.Insert(d);
		}
		return pos.Count();
	}

	vector EntryPos()
	{
		if (m_Entry.m_iDoor >= 0)
			return m_G.W(m_G.m_aDoors[m_Entry.m_iDoor].m_vL);
		return m_G.W(m_Entry.m_vA);
	}

	protected bool StackSpots(DCO_INavHop h, out vector a, out vector b)
	{
		a = vector.Zero;
		b = vector.Zero;
		if (!h || h.m_iDoor < 0)
			return false;
		DCO_INavDoor dr = m_G.m_aDoors[h.m_iDoor];
		float near = vector.Dot(h.m_vA - dr.m_vL, dr.m_vN);
		foreach (DCO_INavPoint p : m_G.m_aPoints)
		{
			if (p.m_iType != 2 || p.m_iDoor != h.m_iDoor || p.m_iOrder != 0)
				continue;
			if (near * vector.Dot(p.m_vL - dr.m_vL, dr.m_vN) <= 0)
				continue;
			if (vector.Dot(p.m_vL - dr.m_vL, dr.m_vS) > 0)
				a = m_G.W(p.m_vL);
			else
				b = m_G.W(p.m_vL);
		}
		return a != vector.Zero || b != vector.Zero;
	}

	void ApplyEntry(inout vector outDir, inout vector stackA, inout vector stackB, inout vector door)
	{
		vector a = m_G.W(m_Entry.m_vA);
		vector o = Flat(a - m_G.W(m_Entry.m_vB));
		if (o != vector.Zero)
			outDir = o;
		vector sa, sb;
		StackSpots(m_Entry, sa, sb);
		if (sa != vector.Zero)
			stackA = sa;
		if (sb != vector.Zero)
			stackB = sb;
		door = a;
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_graph_entry grp=%1 door=%2 real=%3 room=%4 stack_a=%5 stack_b=%6", m_sGrp, m_Entry.m_iDoor, DCO_Util.B01(m_Entry.m_iDoor >= 0 && m_G.m_aDoors[m_Entry.m_iDoor].m_bReal), m_Entry.m_iB, DCO_Util.B01(sa != vector.Zero), DCO_Util.B01(sb != vector.Zero)));
	}

	protected bool Open(array<ref DCO_CQBRoom> rooms, array<int> cqbOf, int g)
	{
		int c = cqbOf[g];
		return c < 0 || rooms[c].m_iState != 0;
	}

	protected ref array<float> m_aRnd = {};

	protected float Score(array<ref DCO_CQBRoom> rooms, int c, int priority)
	{
		float s = c;
		if (c < m_aRnd.Count())
			s = m_aRnd[c];
		s += 100.0 * rooms[c].m_iDefer;
		if (c == priority)
			s -= 1000.0;
		if (m_aBelief && c < m_aBelief.Count())
			s -= 1000.0 * m_aBelief[c];
		if (m_iBeliefFloor >= 0 && rooms[c].m_iFloor == m_iBeliefFloor)
			s -= 200.0;
		return s;
	}

	int Pick(array<ref DCO_CQBRoom> rooms, int foothold, int priority, vector near = vector.Zero)
	{
		m_Hop = null;
		bool pending = false;
		foreach (DCO_CQBRoom pr : rooms)
		{
			if (pr.m_iState == 0)
				pending = true;
		}
		if (!pending)
			return -1;
		m_aRnd.Clear();
		foreach (DCO_CQBRoom rr : rooms)
		{
			m_aRnd.Insert(Math.RandomFloat(0, 10));
		}
		int n = m_G.m_aRooms.Count();
		array<int> cqbOf = {};
		for (int i = 0; i < n; i++)
		{
			cqbOf.Insert(-1);
		}
		foreach (int ci, int gid : m_aGraphRoom)
		{
			if (gid >= 0 && gid < n && ci < rooms.Count())
				cqbOf[gid] = ci;
		}
		int footG = -1;
		if (foothold >= 0 && foothold < m_aGraphRoom.Count())
			footG = m_aGraphRoom[foothold];
		int best = -1;
		float bestS = float.MAX;
		DCO_INavHop bestHop;
		bool bestOpen = false;
		float bestWalk = 0;
		vector refW = m_G.W(m_Entry.m_vA);
		if (footG >= 0 && footG < n)
			refW = RoomCenter(m_G.m_aRooms[footG]);
		if (near != vector.Zero)
			refW = near;
		int er = m_Entry.m_iB;
		array<int> queue = {};
		array<int> seen = {};
		if (er >= 0)
		{
			seen.Insert(er);
			if (Open(rooms, cqbOf, er))
				queue.Insert(er);
			else
			{
				best = cqbOf[er];
				bestS = Score(rooms, best, priority);
				bestHop = m_Entry;
			}
		}
		while (!queue.IsEmpty())
		{
			int u = queue[0];
			queue.Remove(0);
			foreach (DCO_INavHop h : m_aHops)
			{
				int v = -2;
				if (h.m_iA == u)
					v = h.m_iB;
				else if (h.m_iB == u)
					v = h.m_iA;
				if (v < -1)
					continue;
				if (v == -1 || Open(rooms, cqbOf, v))
				{
					if (!seen.Contains(v))
					{
						seen.Insert(v);
						queue.Insert(v);
					}
					continue;
				}
				int c = cqbOf[v];
				float s = Score(rooms, c, priority);
				if (u == footG)
					s -= 0.5;
				float walk = 0;
				if (u == -1)
				{
					walk = vector.DistanceXZ(refW, m_G.W(Oriented(h, -1).m_vA));
					s += 20.0 + 0.5 * walk;
				}
				bool open = DoorOpen(h.m_iDoor);
				if (open)
					s -= 3.0;
				if (s < bestS)
				{
					bestS = s;
					best = c;
					bestHop = Oriented(h, u);
					bestOpen = open;
					bestWalk = walk;
				}
			}
		}
		if (best < 0)
		{
			DCO_Perf.Count("cqb:graph_pick_none");
			DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_graph_pick grp=%1 result=fallback reason=no_reachable_room foothold=%2", m_sGrp, foothold));
			return -2;
		}
		m_Hop = bestHop;
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_graph_pick grp=%1 room=%2 graph_room=%3 from_room=%4 door=%5 foothold=%6 priority=%7 defer=%8 open=%9", m_sGrp, best, m_aGraphRoom[best], bestHop.m_iA, bestHop.m_iDoor, foothold, DCO_Util.B01(best == priority), rooms[best].m_iDefer, DCO_Util.B01(bestOpen)) + string.Format(" walk_m=%1", Math.Round(bestWalk)));
		return best;
	}

	protected bool DoorOpen(int di)
	{
		if (di < 0 || di >= m_G.m_aDoors.Count())
			return false;
		DCO_INavDoor d = m_G.m_aDoors[di];
		if (!d.m_bReal || !d.m_Ent)
			return false;
		BaseDoorComponent door = BaseDoorComponent.Cast(d.m_Ent.FindComponent(BaseDoorComponent));
		return door && door.IsOpen();
	}

	bool HopStairs()
	{
		return m_Hop && IsStairHop(m_Hop);
	}

	bool WallSpot(vector near, float r, out vector spot)
	{
		spot = vector.Zero;
		float best = r;
		foreach (DCO_INavSample s : m_G.m_aSamples)
		{
			if (s.m_iDirAll == 15)
				continue;
			vector w = m_G.W(s.m_vL);
			if (Math.AbsFloat(w[1] - near[1]) > 1.2)
				continue;
			float d = vector.DistanceXZ(w, near);
			if (d > best)
				continue;
			best = d;
			spot = w;
		}
		return spot != vector.Zero;
	}

	bool Approach(int idx, inout vector door, inout vector dir, out vector stA, out vector stB)
	{
		stA = vector.Zero;
		stB = vector.Zero;
		if (!m_Hop || idx < 0 || idx >= m_aGraphRoom.Count() || m_Hop.m_iB != m_aGraphRoom[idx])
			return false;
		vector a = m_G.W(m_Hop.m_vA);
		vector d = Flat(m_G.W(m_Hop.m_vB) - a);
		if (d == vector.Zero)
			return false;
		door = a;
		dir = d;
		StackSpots(m_Hop, stA, stB);
		bool real = m_Hop.m_iDoor >= 0 && m_G.m_aDoors[m_Hop.m_iDoor].m_bReal;
		DCO_Perf.Count("cqb:graph_door");
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_graph_door grp=%1 room=%2 graph_room=%3 from_room=%4 door=%5 real=%6 stairs=%7 stack_a=%8 stack_b=%9", m_sGrp, idx, m_Hop.m_iB, m_Hop.m_iA, m_Hop.m_iDoor, DCO_Util.B01(real), DCO_Util.B01(IsStairHop(m_Hop)), DCO_Util.B01(stA != vector.Zero), DCO_Util.B01(stB != vector.Zero)));
		return true;
	}

	int ThreatLooks(int idx, vector from, out vector a, out vector b)
	{
		a = vector.Zero;
		b = vector.Zero;
		if (idx < 0 || idx >= m_aGraphRoom.Count())
			return 0;
		int gid = m_aGraphRoom[idx];
		int n = 0;
		int la = -1;
		int lb = -1;
		float da = -1;
		float db = -1;
		foreach (DCO_INavPoint p : m_G.m_aPoints)
		{
			if (p.m_iType != 4 || p.m_iRoom != gid)
				continue;
			vector w = m_G.W(p.m_vL);
			float d = vector.DistanceXZ(w, from);
			if (d < 1.5)
				continue;
			n++;
			bool beatA = p.m_iLevel > la || (p.m_iLevel == la && d > da);
			if (beatA)
			{
				b = a;
				lb = la;
				db = da;
				a = w;
				la = p.m_iLevel;
				da = d;
				continue;
			}
			bool beatB = p.m_iLevel > lb || (p.m_iLevel == lb && d > db);
			if (beatB)
			{
				b = w;
				lb = p.m_iLevel;
				db = d;
			}
		}
		if (b == vector.Zero)
			b = a;
		return n;
	}

	bool EntryPicks(int idx, DCO_CQBRoom room, out vector left, out vector right)
	{
		left = vector.Zero;
		right = vector.Zero;
		if (!m_Hop || idx < 0 || idx >= m_aGraphRoom.Count() || m_Hop.m_iB != m_aGraphRoom[idx])
			return false;
		vector b = m_G.W(m_Hop.m_vB);
		vector dir = Flat(b - m_G.W(m_Hop.m_vA));
		if (dir == vector.Zero)
			return false;
		vector side = Vector(dir[2], 0, -dir[0]);
		foreach (vector sp : room.m_aSlots)
		{
			float lat = vector.Dot(sp - b, side);
			if (lat > 0.3 && left == vector.Zero)
				left = sp;
			else if (lat < -0.3 && right == vector.Zero)
				right = sp;
		}
		if (left == vector.Zero && right != vector.Zero)
			left = b + dir * 1.2 + side * 0.8;
		else if (right == vector.Zero && left != vector.Zero)
			right = b + dir * 1.2 - side * 0.8;
		bool ok = left != vector.Zero && right != vector.Zero;
		DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_graph_entry_points grp=%1 room=%2 slots=%3 left=%4 right=%5 used=%6", m_sGrp, idx, room.m_aSlots.Count(), DCO_Util.B01(left != vector.Zero), DCO_Util.B01(right != vector.Zero), DCO_Util.B01(ok)));
		return ok;
	}

	void OpenDoors(array<SCR_ChimeraAIAgent> team, float now)
	{
		foreach (SCR_ChimeraAIAgent a : team)
		{
			if (!a)
				continue;
			IEntity me = a.GetControlledEntity();
			if (!me)
				continue;
			vector p = me.GetOrigin();
			int trackDoor = -1;
			bool tracked = m_mPassDoor.Find(a, trackDoor);
			foreach (int di, DCO_INavDoor d : m_G.m_aDoors)
			{
				if (!d.m_Ent)
					continue;
				vector dw = m_G.W(d.m_vL);
				float dd = vector.DistanceXZ(dw, p);
				bool sameLevel = Math.AbsFloat(dw[1] - p[1]) <= 1.8;
				BaseDoorComponent door = BaseDoorComponent.Cast(d.m_Ent.FindComponent(BaseDoorComponent));
				if (tracked && trackDoor == di && (dd > 2.5 || !sameLevel))
				{
					float started = m_mPassAt.Get(a);
					DCO_Perf.Count("cqb:door_pass");
					DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_door_pass grp=%1 unit=%2 door=%3 dur_s=%4 open=%5", m_sGrp, DCO_Log.IdUnit(me), di, ((now - started) / 1000.0).ToString(-1, 1), DCO_Util.B01(door && door.IsOpen())));
					m_mPassDoor.Remove(a);
					m_mPassAt.Remove(a);
					tracked = false;
				}
				if (!sameLevel)
					continue;
				if (!tracked && dd <= 2.0)
				{
					m_mPassDoor.Set(a, di);
					m_mPassAt.Set(a, now);
					tracked = true;
					trackDoor = di;
				}
				float reach = 4.0;
				if (m_Hop && m_Hop.m_iDoor == di)
					reach = 5.0;
				if (dd > reach)
					continue;
				if (now < m_mDoorTry.Get(di))
					continue;
				if (!door || door.IsOpen() || door.IsOpening() || Math.AbsFloat(door.GetControlValue()) >= 0.5)
					continue;
				m_mDoorTry.Set(di, now + 1000.0);
				door.UseDoorAction(me);
				DCO_Perf.Count("cqb:door_open");
				DCO_Log.Write(DCO_ELogChannel.CQB, DCO_ELogLevel.INFO, "-", string.Format("cqb_door_open grp=%1 door=%2 unit=%3 hop=%4 lead_m=%5", m_sGrp, di, DCO_Log.IdUnit(me), DCO_Util.B01(m_Hop && m_Hop.m_iDoor == di), dd.ToString(-1, 1)));
			}
		}
	}
}
