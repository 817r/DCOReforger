class DCO_CVPainter
{
	protected Widget m_wRoot;
	protected CanvasWidget m_wCanvas;
	protected CanvasWidget m_wPanel;
	protected ref array<Widget> m_aLabels = {};
	protected int m_iLabel;
	protected ref array<ref CanvasWidgetCommand> m_aCmd = {};
	protected ref array<ref CanvasWidgetCommand> m_aPanelCmd = {};
	protected ref array<ref DCO_CVHit> m_aHits = {};
	protected ref array<float> m_aOcc = {};
	protected float m_fDpi = 1;
	protected float m_fSw;
	protected float m_fSh;

	protected float EstW(string text, int size)
	{
		int longest = 0;
		array<string> lines = {};
		text.Split("\n", lines, false);
		foreach (string ln : lines)
			longest = Math.Max(longest, ln.Length());
		return longest * size * 0.52 * m_fDpi;
	}

	protected bool Free(float x0, float y0, float x1, float y1)
	{
		for (int i = 0; i + 3 < m_aOcc.Count(); i += 4)
		{
			if (x0 < m_aOcc[i + 2] && x1 > m_aOcc[i] && y0 < m_aOcc[i + 3] && y1 > m_aOcc[i + 1])
				return false;
		}
		return true;
	}

	protected void Occupy(float x0, float y0, float x1, float y1)
	{
		m_aOcc.Insert(x0);
		m_aOcc.Insert(y0);
		m_aOcc.Insert(x1);
		m_aOcc.Insert(y1);
	}

	protected Widget Lbl(float sx, float sy, string text, int color, int size, int halo)
	{
		WorkspaceWidget ws = GetGame().GetWorkspace();
		if (m_iLabel >= m_aLabels.Count())
		{
			Widget w = ws.CreateWidgets("{A7E21C4B93D85F12}UI/layouts/DCO/DCO_CmdViewLabel.layout", m_wRoot);
			if (!w)
				return null;
			m_aLabels.Insert(w);
		}
		Widget lw = m_aLabels[m_iLabel];
		m_iLabel++;
		RichTextWidget t = RichTextWidget.Cast(lw);
		if (t)
		{
			t.SetText(text);
			t.SetColorInt(DCO_CmdViewUI.Lin(color));
			t.SetExactFontSize(size);
			if (halo != 0)
				t.SetOutline(2, DCO_CmdViewUI.Lin(halo));
			else
				t.SetOutline(0, DCO_CmdViewUI.Lin(color));
		}
		lw.SetVisible(true);
		FrameSlot.SetPos(lw, ws.DPIUnscale(sx), ws.DPIUnscale(sy));
		FrameSlot.SetSize(lw, ws.DPIUnscale(EstW(text, size) * 1.7 + 24.0 * m_fDpi), ws.DPIUnscale(1.0 * size * 1.35 * m_fDpi));
		return lw;
	}

	protected void Hit(int type, int ci, string id, float x0, float y0, float x1, float y1, string tip, int color, float wx, float wz)
	{
		DCO_CVHit h = new DCO_CVHit();
		h.m_iType = type;
		h.m_iCi = ci;
		h.m_sId = id;
		h.m_fX0 = x0;
		h.m_fY0 = y0;
		h.m_fX1 = x1;
		h.m_fY1 = y1;
		h.m_sTip = tip;
		h.m_iColor = color;
		h.m_fWX = wx;
		h.m_fWZ = wz;
		m_aHits.Insert(h);
	}

	protected static array<string> Wrap(string text, int maxChars)
	{
		array<string> lines = {};
		array<string> words = {};
		text.Split(" ", words, true);
		string cur = "";
		foreach (string w : words)
		{
			if (!cur.IsEmpty() && cur.Length() + 1 + w.Length() > maxChars)
			{
				lines.Insert(cur);
				cur = w;
				continue;
			}
			if (cur.IsEmpty())
				cur = w;
			else
				cur += " " + w;
		}
		if (!cur.IsEmpty() || lines.IsEmpty())
			lines.Insert(cur);
		return lines;
	}

	protected static string Cut(string text, int max)
	{
		if (text.Length() <= max)
			return text;
		return text.Substring(0, max - 3) + "...";
	}

	float DrawCmdCard(DCO_CVCmd c, float x, float y, bool pinned, bool sel)
	{
		float d = m_fDpi;
		array<string> lines = {};
		array<bool> head = {};
		lines.Insert(Cut(c.m_sNow, 74));
		head.Insert(false);
		array<string> part = {};
		if (!c.m_sPlans.IsEmpty())
		{
			lines.Insert("Plans:");
			head.Insert(true);
			c.m_sPlans.Split("\n", part, true);
			foreach (string pl : part)
			{
				lines.Insert("  " + Cut(pl, 72));
				head.Insert(false);
			}
		}
		if (!c.m_sDecs.IsEmpty())
		{
			lines.Insert("Last decisions:");
			head.Insert(true);
			part.Clear();
			c.m_sDecs.Split("\n", part, true);
			foreach (string dl : part)
			{
				lines.Insert("  " + Cut(dl, 72));
				head.Insert(false);
			}
		}
		lines.Insert(Cut(c.m_sForces, 74));
		head.Insert(false);
		string title = Cut(c.m_sTitle, 60);
		float w = EstW(title, 18);
		foreach (string ln : lines)
			w = Math.Max(w, EstW(ln, 16));
		w = Math.Min(w + 28.0 * d, 660.0 * d);
		float h = (34.0 + lines.Count() * 20.0 + 8.0) * d;
		DCO_CmdViewUI.Poly(m_aPanelCmd, DCO_CmdViewUI.Rect(x, y, x + w, y + h), 0xEB1E2225);
		DCO_CmdViewUI.Poly(m_aPanelCmd, DCO_CmdViewUI.Rect(x, y, x + 4.0 * d, y + h), c.m_iColor);
		if (sel || pinned)
			DCO_CmdViewUI.Line(m_aPanelCmd, DCO_CmdViewUI.Rect(x, y, x + w, y + h), c.m_iColor, 2, true);
		Lbl(x + 14.0 * d, y + 6.0 * d, title, 0xFFF1F1EF, 18, 0);
		float ly = y + 34.0 * d;
		foreach (int i, string tl : lines)
		{
			int col = 0xFFC9CCD0;
			if (head[i])
				col = 0xFF9AA0A5;
			Lbl(x + 14.0 * d, ly, tl, col, 16, 0);
			ly += 20.0 * d;
		}
		Occupy(x, y, x + w, y + h);
		Hit(4, c.m_iIdx, c.m_iIdx.ToString() + ":cmd", x, y, x + w, y + h, "", c.m_iColor, 0, 0);
		return h;
	}

	protected string m_sRowsLayoutSig;

	protected int MaxChars(float px, int size)
	{
		return Math.Max(8, Math.Floor(px / (1.0 * size * 0.52 * m_fDpi)) - 1);
	}

	protected array<string> WrapAll(string text, int maxChars, out int wrapped)
	{
		array<string> outL = {};
		array<string> parts = {};
		text.Split("\n", parts, false);
		foreach (string part : parts)
		{
			array<string> wl = Wrap(part, maxChars);
			if (wl.Count() > 1)
				wrapped++;
			outL.InsertAll(wl);
		}
		return outL;
	}

	void DrawRows(array<string> rows, float x0, float y0, float x1, float y1, int ccol, int selCi)
	{
		float d = m_fDpi;
		int wrapped = 0;
		int labelStart = m_iLabel;
		float right = x1 - 18.0 * d;
		DCO_CmdViewUI.Poly(m_aPanelCmd, DCO_CmdViewUI.Rect(x0, y0, x1, y1), 0xF51E2225);
		float px = x0 + 18.0 * d;
		float y = y0 + 12.0 * d;
		float maxY = y1 - 40.0 * d;
		bool headerDone = false;
		if (rows.IsEmpty())
		{
			Lbl(px, y, "Loading...", 0xFF9AA0A5, 18, 0);
		}
		foreach (string row : rows)
		{
			array<string> p = {};
			row.Split("\t", p, false);
			if (p.Count() < 2)
				continue;
			string st = p[0];
			string a = p[1];
			string b = "";
			if (p.Count() > 2)
				b = p[2];
			if (!headerDone && st != "K" && st != "T" && st != "S" && st != "C")
			{
				headerDone = true;
				y += 4.0 * d;
				DCO_CmdViewUI.Poly(m_aPanelCmd, DCO_CmdViewUI.Rect(x0, y, x1, y + 3.0 * d), ccol);
				y += 12.0 * d;
			}
			if (y > maxY)
				break;
			switch (st)
			{
				case "K":
				{
					string k = a;
					k.ToUpper();
					Lbl(px, y, k, 0xFF9AA0A5, 14, 0);
					y += 20.0 * d;
					break;
				}
				case "T":
				{
					foreach (string tln : WrapAll(a, MaxChars(right - px, 24), wrapped))
					{
						Lbl(px, y, tln, 0xFFF1F1EF, 24, 0);
						y += 32.0 * d;
					}
					break;
				}
				case "S":
				{
					foreach (string sln : WrapAll(a, MaxChars(right - px, 18), wrapped))
					{
						Lbl(px, y, sln, 0xFFC9CCD0, 18, 0);
						y += 26.0 * d;
					}
					break;
				}
				case "C":
				{
					int cb = ccol;
					if (a == "SKIP")
						cb = 0xFF6B6B6B;
					float cw = EstW(a, 16) + 14.0 * d;
					DCO_CmdViewUI.Poly(m_aPanelCmd, DCO_CmdViewUI.Rect(px, y, px + cw, y + 22.0 * d), cb);
					Lbl(px + 7.0 * d, y + 1.0 * d, a, 0xFFFFFFFF, 16, 0);
					Lbl(px + cw + 10.0 * d, y + 1.0 * d, "band " + b, 0xFFC9CCD0, 16, 0);
					y += 30.0 * d;
					break;
				}
				case "W":
				{
					array<string> wl = WrapAll(a, MaxChars(right - px - 20.0 * d, 18), wrapped);
					float wh = wl.Count() * 24.0 * d + 10.0 * d;
					DCO_CmdViewUI.Poly(m_aPanelCmd, DCO_CmdViewUI.Rect(px, y, x1 - 18.0 * d, y + wh), 0x24E3A13B);
					DCO_CmdViewUI.Poly(m_aPanelCmd, DCO_CmdViewUI.Rect(px, y, px + 3.0 * d, y + wh), 0xFFE3A13B);
					float wy = y + 5.0 * d;
					foreach (string wln : wl)
					{
						Lbl(px + 12.0 * d, wy, wln, 0xFFF1F1EF, 18, 0);
						wy += 24.0 * d;
					}
					y += wh + 10.0 * d;
					break;
				}
				case "H":
				{
					string hh = a;
					hh.ToUpper();
					y += 4.0 * d;
					Lbl(px, y, hh, 0xFF9AA0A5, 14, 0);
					y += 19.0 * d;
					DCO_CmdViewUI.Poly(m_aPanelCmd, DCO_CmdViewUI.Rect(px, y, x1 - 18.0 * d, y + 1.0 * d), 0x24FFFFFF);
					y += 5.0 * d;
					break;
				}
				case "V":
				{
					Lbl(px, y + 2.0 * d, a, 0xFF9AA0A5, 16, 0);
					array<string> vl = WrapAll(b, MaxChars(right - px - 140.0 * d, 18), wrapped);
					if (vl.IsEmpty())
						y += 24.0 * d;
					foreach (string vln : vl)
					{
						Lbl(px + 140.0 * d, y, vln, 0xFFF1F1EF, 18, 0);
						y += 24.0 * d;
					}
					break;
				}
				case "G":
				{
					array<string> gl = WrapAll(b, MaxChars(right - px, 18), wrapped);
					float gy0 = y;
					foreach (string gln : gl)
					{
						Lbl(px, y, gln, 0xFFF1F1EF, 18, 0);
						y += 24.0 * d;
					}
					DCO_CmdViewUI.Poly(m_aPanelCmd, DCO_CmdViewUI.Rect(px - 4.0 * d, gy0 + 22.0 * d * gl.Count(), x1 - 18.0 * d, gy0 + 22.0 * d * gl.Count() + 1.0 * d), 0x40E3A13B);
					Hit(20, selCi, selCi.ToString() + ":" + a, px - 4.0 * d, gy0, x1 - 18.0 * d, y, "", 0, 0, 0);
					y += 4.0 * d;
					break;
				}
				case "O":
				{
					array<string> ol = WrapAll(b, MaxChars(right - px, 18), wrapped);
					float oy0 = y;
					foreach (string oln : ol)
					{
						Lbl(px, y, oln, 0xFFF1F1EF, 18, 0);
						y += 24.0 * d;
					}
					Hit(22, selCi, a, px - 4.0 * d, oy0, x1 - 18.0 * d, y, "", 0, 0, 0);
					y += 4.0 * d;
					break;
				}
				case "D":
				{
					Lbl(px, y + 1.0 * d, a, 0xFF9AA0A5, 16, 0);
					array<string> dl = WrapAll(b, MaxChars(right - px - 76.0 * d, 18), wrapped);
					foreach (string dln : dl)
					{
						Lbl(px + 76.0 * d, y, dln, 0xFFC9CCD0, 18, 0);
						y += 23.0 * d;
					}
					break;
				}
				default:
				{
					foreach (string nln : WrapAll(a, MaxChars(right - px, 16), wrapped))
					{
						Lbl(px, y, nln, 0xFF9AA0A5, 16, 0);
						y += 22.0 * d;
					}
					break;
				}
			}
		}
		int lines = m_iLabel - labelStart;
		string layoutSig = string.Format("%1|%2|%3", Math.Round(x1 - x0), rows.Count(), lines);
		if (layoutSig != m_sRowsLayoutSig)
		{
			m_sRowsLayoutSig = layoutSig;
			if (DCO_Log.On(DCO_ELogChannel.SYS, DCO_ELogLevel.DEBUG))
				DCO_Log.Write(DCO_ELogChannel.SYS, DCO_ELogLevel.DEBUG, "-", string.Format("cmdview_panel_layout painter=%1 width=%2 rows=%3 lines=%4 wrapped=%5 height_px=%6 x0=%7 y0=%8", ClassName(), Math.Round(x1 - x0), rows.Count(), lines, wrapped, Math.Round(y - y0), Math.Round(x0), Math.Round(y0)));
		}
		DCO_CmdViewUI.Poly(m_aPanelCmd, DCO_CmdViewUI.Rect(x0, y1 - 34.0 * d, x1, y1 - 33.0 * d), 0x24FFFFFF);
		Lbl(px, y1 - 28.0 * d, "Esc  Close  ·  Click a name to jump", 0xFF9AA0A5, 15, 0);
	}

}
