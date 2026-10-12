const fs = require("fs");
const path = require("path");

const USAGE = `usage: node tools/dco-abmetrics.js --code <ITEST code prefix> [options]
  --code PERF_STRESS         base code; arms are <code>_<suffix> (e.g. PERF_STRESS_ABL_PANIC30 -> PANIC30); the code itself is BASE
  --day 2026-10-10           log day folder (repeatable via comma: 2026-10-09,2026-10-10)
  --since session_063000     skip sessions with an earlier folder name (per day)
  --until session_235959     skip sessions with a later folder name (per day)
  --sessions a,b,c           only these session folder names
  --mindur 590               minimum battle duration_s (drops aborted runs)
  --runs                     also print one line per run
  --json                     print JSON instead of a table
  --logs <dir>               log root (default Workbench profile/DCO/Logs)
Standard metrics per arm (mean over runs): deaths (unit_summary state=dead, or death_ctx count for runs before 2026-10-10 23:30), unconEnd (unconscious when the run ended, not dead), open deaths, FF (and % explosive), prone-still deaths, died in PANIC,
panic count and median duration, rounds/unit, hits, first death t, half-death t, cover% median, kill distance p50 and % < 25 m (infantry),
losses per side from SUMMARY lines, kill distance p50 per victim faction (kP50vUS / kP50vRU).
Kill distance excludes deaths whose cause contains vehicle (vehicle_fire, vehicle_collision), idf or no_attacker (falls/environment), and any dist < 0.`;

const args = {};
for (let i = 2; i < process.argv.length; i++) {
  const a = process.argv[i];
  if (!a.startsWith("--")) continue;
  const k = a.slice(2);
  const next = process.argv[i + 1];
  if (next === undefined || next.startsWith("--")) args[k] = true;
  else { args[k] = next; i++; }
}
if (!args.code || args.help) { console.log(USAGE); process.exit(args.code ? 0 : 1); }

const ROOT = args.logs || path.join(process.env.USERPROFILE || "C:/Users/r9445", "OneDrive", "文档", "My Games", "ArmaReforgerWorkbench", "profile", "DCO", "Logs");
const days = String(args.day || new Date().toISOString().slice(0, 10)).split(",");
const minDur = +(args.mindur || 0);
const only = args.sessions ? new Set(String(args.sessions).split(",")) : null;
const codeRe = new RegExp("itest_start code=(" + args.code + "(?:_\\S*)?)(\\s|$)");

const med = a => { if (!a.length) return 0; const s = a.slice().sort((x, y) => x - y); return s[s.length >> 1]; };
const mean = a => a.length ? a.reduce((x, y) => x + y, 0) / a.length : 0;
const isFF = x => x.cause === "fratricide" || (x.vfac === "US" && /_US_/.test(x.kname || "")) || (x.vfac === "USSR" && /USSR/.test(x.kname || ""));

function runMetrics(dir) {
  const log = fs.readFileSync(path.join(dir, "dco.log"), "utf8");
  const m = codeRe.exec(log);
  if (!m) return null;
  let j;
  try { j = JSON.parse(fs.readFileSync(path.join(dir, "battle.json"), "utf8")); } catch (e) { return null; }
  if ((j.duration_s || 0) < minDur) return null;
  const D = j.death_ctx || [];
  const U = j.unit_summary || [];
  const inf = D.filter(x => !/vehicle|idf|no_attacker/.test(x.cause || "") && typeof x.dist === "number" && x.dist >= 0);
  const ff = D.filter(isFF);
  let panicN = 0;
  const start = {}, durs = [];
  for (const l of log.split(/\r?\n/)) {
    const q = /T\+([\d.]+) .*morale_state unit=(\S+) from=(\S+) to=(\S+)/.exec(l);
    if (!q) continue;
    const t = +q[1];
    if (q[4] === "PANIC" && q[3] !== "BREAK") { panicN++; start[q[2]] = t; }
    if (q[3] === "PANIC" && start[q[2]] != null) { durs.push(t - start[q[2]]); delete start[q[2]]; }
  }
  const lost = fac => +((new RegExp("SUMMARY faction=" + fac + " .*?infantry_lost=(\\d+)").exec(log) || [])[1] || 0);
  const code = m[1];
  return {
    arm: code === args.code ? "BASE" : code.replace(args.code + "_ABL_", "").replace(args.code + "_", ""),
    session: path.basename(dir),
    duration_s: j.duration_s,
    deaths: U.some(u => u.state) ? U.filter(u => u.state === "dead" || u.state === "dead_unlogged").length : D.length,
    uncon_end: U.some(u => u.state) ? U.filter(u => u.state === "uncon").length : Math.max(0, U.filter(u => !u.alive).length - D.length),
    ctx_n: D.length,
    open: D.filter(x => x.vcover === "none" && !x.vindoor).length,
    ff: ff.length,
    ff_expl_pct: ff.length ? Math.round(100 * ff.filter(x => /EXPLOSIVE|FRAGMENTATION|INCENDIARY/.test(x.dmg || "")).length / ff.length) : 0,
    ff_veh: ff.filter(x => /COLLISION/.test(x.dmg || "") || /collision/.test(x.cause || "")).length,
    prone_still: D.filter(x => x.vstance === "PRONE" && !x.vmoving).length,
    moving: D.filter(x => x.vmoving).length,
    killer_seen_pct: D.length ? Math.round(100 * D.filter(x => x.seen).length / D.length) : 0,
    kills: U.reduce((a, u) => a + (u.kills || 0), 0),
    died_panic: D.filter(x => x.vmorale === "PANIC").length,
    panic_n: panicN,
    panic_dur_med_s: Math.round(med(durs)),
    rounds_unit: +(mean(U.map(u => u.rounds || 0))).toFixed(1),
    hits: U.reduce((a, u) => a + (u.hits || 0), 0),
    first_death_s: D.length ? Math.round(Math.min(...D.map(x => x.t))) : 0,
    half_death_s: Math.round(med(D.map(x => x.t))),
    cover_pct_med: med(U.map(u => u.cover_pct || 0)),
    kill_p50_m: Math.round(med(inf.map(x => x.dist))),
    kill_lt25_pct: inf.length ? Math.round(100 * inf.filter(x => x.dist < 25).length / inf.length) : 0,
    kill_p50_us_m: Math.round(med(inf.filter(x => x.vfac === "US").map(x => x.dist))),
    kill_p50_ussr_m: Math.round(med(inf.filter(x => x.vfac === "USSR").map(x => x.dist))),
    lost_us: lost("US"),
    lost_ussr: lost("USSR")
  };
}

const runs = [];
for (const day of days) {
  const dayDir = path.join(ROOT, day);
  if (!fs.existsSync(dayDir)) continue;
  for (const bucket of fs.readdirSync(dayDir)) {
    const bdir = path.join(dayDir, bucket);
    if (!fs.statSync(bdir).isDirectory()) continue;
    for (const s of fs.readdirSync(bdir)) {
      if (!s.startsWith("session_")) continue;
      if (args.since && s < args.since) continue;
      if (args.until && s > args.until) continue;
      if (only && !only.has(s)) continue;
      const dir = path.join(bdir, s);
      if (!fs.existsSync(path.join(dir, "battle.json")) || !fs.existsSync(path.join(dir, "dco.log"))) continue;
      const r = runMetrics(dir);
      if (r) { r.day = day; runs.push(r); }
    }
  }
}

const keys = ["deaths", "uncon_end", "open", "ff", "ff_expl_pct", "ff_veh", "prone_still", "moving", "killer_seen_pct", "kills", "died_panic", "panic_n", "panic_dur_med_s", "rounds_unit", "hits", "first_death_s", "half_death_s", "cover_pct_med", "kill_p50_m", "kill_lt25_pct", "kill_p50_us_m", "kill_p50_ussr_m", "lost_us", "lost_ussr"];
const arms = {};
for (const r of runs) (arms[r.arm] = arms[r.arm] || []).push(r);
const summary = Object.keys(arms).sort((a, b) => (a === "BASE" ? -1 : b === "BASE" ? 1 : a.localeCompare(b))).map(a => {
  const o = { arm: a, n: arms[a].length };
  for (const k of keys) o[k] = +mean(arms[a].map(r => r[k])).toFixed(1);
  o.sessions = arms[a].map(r => r.session);
  return o;
});

if (args.json) {
  console.log(JSON.stringify({ code: args.code, days, summary, runs: args.runs ? runs : undefined }, null, 1));
} else {
  const short = { deaths: "deaths", uncon_end: "unconEnd", open: "open", ff: "ff", ff_expl_pct: "ffExp%", ff_veh: "ffVeh", prone_still: "pStill", moving: "moving", killer_seen_pct: "seen%", kills: "kills", died_panic: "diedPan", panic_n: "panicN", panic_dur_med_s: "panDur", rounds_unit: "rnd/u", hits: "hits", first_death_s: "1stD", half_death_s: "halfD", cover_pct_med: "cov%", kill_p50_m: "kP50", kill_lt25_pct: "k<25%", kill_p50_us_m: "kP50vUS", kill_p50_ussr_m: "kP50vRU", lost_us: "lostUS", lost_ussr: "lostRU" };
  console.log("arm".padEnd(12) + " n " + keys.map(k => short[k].padStart(9)).join(""));
  for (const o of summary) {
    console.log(o.arm.padEnd(12) + String(o.n).padStart(2) + " " + keys.map(k => String(o[k]).padStart(9)).join(""));
    if (args.runs) for (const r of arms[o.arm]) console.log("  " + (r.day + "/" + r.session).padEnd(34) + keys.map(k => String(r[k]).padStart(9)).join("").slice(0));
    else console.log("  " + o.sessions.join(" "));
  }
  if (!summary.length) console.log("no runs matched code=" + args.code + " days=" + days.join(","));
  const old = runs.filter(r => r.day < "2026-10-10" || (r.day === "2026-10-10" && r.session < "session_045100"));
  if (old.length) console.log("WARNING: " + old.length + " run(s) predate build 2026-10-10 04:51 (finisher/bleed fix); unseen_shooter/bleed fields are not comparable: " + old.map(r => r.day + "/" + r.session).join(" "));
}
