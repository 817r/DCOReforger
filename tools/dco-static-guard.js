#!/usr/bin/env node
"use strict";
const fs = require("fs");
const path = require("path");

const ROOTS = [
  { name: "DCO_AI", dir: "D:/Git/DCOReforger/DCO_AI/scripts", server: true },
  { name: "TestAI", dir: "C:/Users/r9445/OneDrive/文档/My Games/ArmaReforgerWorkbench/addons/TestAI/Scripts", server: false },
];
const BASELINE = path.join(__dirname, "static-guard-baseline.json");
const HEADROOM = 542;

function walk(dir, out) {
  let ents;
  try { ents = fs.readdirSync(dir, { withFileTypes: true }); } catch (e) { return out; }
  for (const e of ents) {
    const p = path.join(dir, e.name);
    if (e.isDirectory()) walk(p, out);
    else if (e.name.endsWith(".c")) out.push(p);
  }
  return out;
}

function blank(src) {
  let out = "";
  let i = 0;
  const n = src.length;
  while (i < n) {
    const c = src[i], d = src[i + 1];
    if (c === "/" && d === "/") { while (i < n && src[i] !== "\n") { out += " "; i++; } continue; }
    if (c === "/" && d === "*") { while (i < n && !(src[i] === "*" && src[i + 1] === "/")) { out += src[i] === "\n" ? "\n" : " "; i++; } out += "  "; i += 2; continue; }
    if (c === '"') { out += '"'; i++; while (i < n && src[i] !== '"') { if (src[i] === "\\") { out += "  "; i += 2; continue; } out += src[i] === "\n" ? "\n" : "x"; i++; } out += '"'; i++; continue; }
    out += c; i++;
  }
  return out;
}

function topLevelElements(body) {
  let depth = 0, count = 0, any = false;
  for (const ch of body) {
    if (ch === "{" || ch === "(" || ch === "[") depth++;
    else if (ch === "}" || ch === ")" || ch === "]") depth--;
    else if (ch === "," && depth === 0) count++;
    if (!/\s/.test(ch)) any = true;
  }
  return any ? count + 1 : 0;
}

function scanFile(file, rootDir) {
  const raw = fs.readFileSync(file, "utf8");
  const src = blank(raw);
  const rel = path.relative(rootDir, file).split(path.sep).join("/");
  const found = [];
  const re = /(^|[;{}\n])\s*((?:(?:protected|private|static|const|ref|autoptr)\s+)+)/g;
  let m;
  while ((m = re.exec(src))) {
    const mods = m[2];
    const isStatic = /\bstatic\b/.test(mods);
    const isConst = /\bconst\b/.test(mods);
    if (!isStatic && !isConst) continue;
    const start = m.index + m[0].length - mods.length;
    let i = re.lastIndex, paren = 0, brace = 0, eq = -1, kind = null;
    for (; i < src.length; i++) {
      const ch = src[i];
      if (ch === "(") { if (eq < 0 && paren === 0 && brace === 0) { kind = "func"; break; } paren++; }
      else if (ch === ")") paren--;
      else if (ch === "{") { if (eq < 0) { kind = "func"; break; } brace++; }
      else if (ch === "}") brace--;
      else if (ch === "=" && eq < 0 && paren === 0 && brace === 0 && src[i + 1] !== "=") eq = i;
      else if (ch === ";" && paren === 0 && brace === 0) break;
    }
    if (kind === "func" || eq < 0) continue;
    const stmt = src.slice(start, i).replace(/\s+/g, " ").trim();
    const init = src.slice(eq + 1, i).trim();
    let weight = 1, elems = 0, cost;
    if (init.startsWith("{")) {
      const inner = init.slice(1, init.lastIndexOf("}"));
      elems = topLevelElements(inner);
      weight += elems;
      cost = 1.0 + 0.5 * elems;
    } else if (/^new\b/.test(init)) cost = 1.0;
    else if (/^(-?\d+(\.\d+)?f?|"x*"|true|false|null|[A-Za-z_]\w*\.[A-Za-z_]\w*)$/.test(init)) cost = 0;
    else cost = 0.5;
    const line = src.slice(0, start).split("\n").length;
    const k = isStatic && isConst ? "static_const" : isStatic ? (/\bref\b/.test(mods) ? "static_ref" : "static") : "const";
    found.push({ file: rel, line, kind: k, weight, elems, cost, sig: rel + "|" + stmt, text: raw.split("\n")[line - 1].replace(/\s+/g, " ").trim() });
    re.lastIndex = i;
  }
  return found;
}

function scanAll() {
  const res = { roots: {}, entries: [] };
  for (const r of ROOTS) {
    const files = walk(r.dir, []);
    let total = 0, count = 0, cost = 0;
    const kinds = {};
    for (const f of files) {
      for (const e of scanFile(f, r.dir)) {
        e.root = r.name;
        e.sig = r.name + ":" + e.sig;
        res.entries.push(e);
        total += e.weight; count++; cost += e.cost;
        kinds[e.kind] = (kinds[e.kind] || 0) + e.weight;
      }
    }
    res.roots[r.name] = { files: files.length, decls: count, weight: total, cost: Math.round(cost * 10) / 10, kinds, server: r.server };
  }
  return res;
}

function main() {
  const args = process.argv.slice(2);
  const scanArg = args.find(a => a.startsWith("--scan="));
  if (scanArg) {
    const dir = scanArg.slice(7);
    let w = 0, c = 0, n = 0;
    for (const f of walk(dir, [])) for (const e of scanFile(f, dir)) { w += e.weight; c += e.cost; n++; }
    console.log(`static-guard scan ${dir}: decls=${n} weight=${w} pool cost=${c.toFixed(1)} units`);
    return 0;
  }
  const cur = scanAll();
  const update = args.includes("--update-baseline");
  const budgetArg = args.find(a => a.startsWith("--budget="));
  let base = null;
  try { base = JSON.parse(fs.readFileSync(BASELINE, "utf8")); } catch (e) {}
  if (update || !base) {
    const budget = budgetArg ? +budgetArg.split("=")[1] : (base && base.budget) || cur.roots.DCO_AI.weight;
    const out = { created: new Date().toISOString(), note: "Static initializer census. Every static/const with an initializer runs in one generated init function per script module (Enfusion limit, 'Too many instructions per function'). weight = 1 + elements of an array/map literal.", budget, totals: cur.roots, entries: cur.entries.map(e => ({ root: e.root, file: e.file, line: e.line, kind: e.kind, weight: e.weight, cost: e.cost, sig: e.sig, text: e.text })), calibration: { game: "1.8.0.13", date: "2026-10-10", unit: "one static ref map<string,int> = new map<string,int>()", headroom_vanilla_units: HEADROOM, costs: { literal: 0, new: 1.0, array_literal: "1.0 + 0.5 per element (int or string)", expression_e_g_0_25_mul_PI: 0.5 }, probe: "isolated scratchpad addon, bisection of N declarations until 'Too many instructions per function'" } };
    fs.writeFileSync(BASELINE, JSON.stringify(out, null, 1));
    console.log(`static-guard: baseline ${update ? "updated" : "created"} DCO_AI weight=${cur.roots.DCO_AI.weight} decls=${cur.roots.DCO_AI.decls} TestAI weight=${cur.roots.TestAI.weight} budget=${budget}`);
    return 0;
  }
  const budget = budgetArg ? +budgetArg.split("=")[1] : base.budget;
  const baseSigs = new Map();
  for (const e of base.entries) baseSigs.set(e.sig, (baseSigs.get(e.sig) || 0) + e.weight);
  const curSigs = new Map();
  for (const e of cur.entries) curSigs.set(e.sig, (curSigs.get(e.sig) || 0) + e.weight);
  const added = cur.entries.filter(e => !baseSigs.has(e.sig) || curSigs.get(e.sig) > baseSigs.get(e.sig));
  const removed = base.entries.filter(e => !curSigs.has(e.sig));
  let fail = false;
  for (const r of Object.keys(cur.roots)) {
    const c = cur.roots[r].weight, b = base.totals[r] ? base.totals[r].weight : 0;
    const k = Object.entries(cur.roots[r].kinds).map(([a, v]) => a + "=" + v).join(" ");
    const cc = cur.roots[r].cost, bc = base.totals[r] && base.totals[r].cost !== undefined ? base.totals[r].cost : cc;
    console.log(`static-guard ${r}: weight=${c} (baseline ${b}, ${c - b >= 0 ? "+" : ""}${c - b}) decls=${cur.roots[r].decls} files=${cur.roots[r].files} [${k}]`);
    console.log(`static-guard ${r}: pool cost=${cc} units (baseline ${bc}); 1 unit = one "static ref X = new X()"; vanilla 1.8.0.13 leaves ~${HEADROOM} units for all mods together`);
    if (c > b || cc > bc + 0.01) fail = true;
  }
  const dco = cur.roots.DCO_AI.weight;
  if (dco > budget) { console.log(`static-guard: DCO_AI weight ${dco} > budget ${budget}`); fail = true; }
  if (added.length) {
    console.log(`static-guard: ${added.length} new or grown initializer(s):`);
    for (const e of added) console.log(`  + ${e.root}/${e.file}:${e.line} [${e.kind} w=${e.weight} cost=${e.cost}] ${e.text.slice(0, 140)}`);
  }
  if (removed.length) console.log(`static-guard: ${removed.length} initializer(s) gone since baseline (run with --update-baseline to lower it)`);
  if (args.includes("--top")) {
    const per = {};
    for (const e of cur.entries) if (e.root === "DCO_AI") { const p = per[e.file] || (per[e.file] = { w: 0, c: 0 }); p.w += e.weight; p.c += e.cost; }
    console.log("  weight  cost  file");
    Object.entries(per).sort((a, b) => b[1].c - a[1].c || b[1].w - a[1].w).slice(0, 25).forEach(([f, p]) => console.log(`  ${String(p.w).padStart(6)} ${p.c.toFixed(1).padStart(5)}  ${f}`));
  }
  console.log(fail ? "static-guard: FAIL (no new static initializers allowed; use literals, instance members, lazy init or .conf/JSON)" : "static-guard: OK");
  return fail ? 1 : 0;
}

process.exit(main());
