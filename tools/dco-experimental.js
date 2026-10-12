const fs = require('fs');
const path = require('path');
const { execSync } = require('child_process');

const REPO = path.join(__dirname, '..');
const ROOT = path.join(REPO, 'DCO_AI');
const DOC = path.join(REPO, 'docs', 'DCO_Experimental_Features.md');
const HELP = path.join(ROOT, 'Configs/DCO/DCO_SettingsHelp.conf');
const BASE = process.env.DCO_EXP_BASE || 'HEAD';
const DRY = process.argv.includes('--dry');
const DOC_ONLY = process.argv.includes('--doc-only');

const git = (args, input) => execSync('git ' + args, { cwd: REPO, input, maxBuffer: 1 << 28, stdio: ['pipe', 'pipe', 'ignore'] }).toString();
const rel = p => path.relative(REPO, p).replace(/\\/g, '/');

function walk(dir, out = []) {
	for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
		const p = path.join(dir, e.name);
		if (e.isDirectory()) walk(p, out);
		else if (p.endsWith('.c')) out.push(p);
	}
	return out;
}
const files = walk(path.join(ROOT, 'scripts')).map(p => ({ p: rel(p), s: fs.readFileSync(p, 'utf8') }));

const changed = new Set(git('-c core.safecrlf=false diff --ignore-cr-at-eol --name-only ' + BASE + ' -- DCO_AI/scripts').split('\n').filter(Boolean));
for (const f of git('ls-files --others --exclude-standard DCO_AI/scripts').split('\n').filter(Boolean)) changed.add(f);

const baseList = git('ls-tree -r --name-only ' + BASE + ' DCO_AI/scripts').split('\n').filter(f => f.endsWith('.c'));
const baseC = git('cat-file --batch', baseList.map(f => BASE + ':' + f).join('\n') + '\n');
const baseEdit = git('show ' + BASE + ':DCO_AI/Configs/Editor/AttributeLists/Edit.conf');
const baseClasses = new Set([...baseEdit.matchAll(/^  ([A-Za-z_0-9]+) "\{[0-9A-F]+\}" \{/gm)].map(m => m[1]));

const B = '(?<![A-Za-z0-9_])', W = '(?![A-Za-z0-9_])';
const esc = s => s.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
const registration = /\/GM\/|\/Settings\/|DCO_GlobalTune[A-Za-z]*\.c$|TuneAttributes|CommanderPresets|Serializer/;

function attributes() {
	const out = [];
	let cur = null;
	for (const l of fs.readFileSync(path.join(ROOT, 'Configs/Editor/AttributeLists/Edit.conf'), 'utf8').replace(/\r\n/g, '\n').split('\n')) {
		const m = l.match(/^  ([A-Za-z_0-9]+) "\{[0-9A-F]+\}" \{$/);
		if (m) { cur = { cls: m[1], name: '', cat: '' }; out.push(cur); continue; }
		if (!cur) continue;
		const n = l.match(/^    Name "(.*)"$/);
		if (n && !cur.name) cur.name = n[1];
		const c = l.match(/^   m_CategoryConfig "\{[0-9A-F]+\}(.*)"$/);
		if (c) cur.cat = path.basename(c[1], '.conf');
	}
	return out;
}

function classBody(cls) {
	const re = new RegExp('class ' + cls + W);
	const f = files.find(x => re.test(x.s));
	if (!f) return null;
	const i = f.s.search(re);
	return { file: f.p, body: f.s.slice(i, i + 3000).split(/\n(?:modded )?class |\n\[BaseContainerProps/)[0] };
}

function readersOf(token) {
	const re = new RegExp(B + esc(token) + W);
	return files.filter(f => !registration.test(f.p) && re.test(f.s)).map(f => f.p);
}

function resolve(cls) {
	const cb = classBody(cls);
	if (!cb) return { how: 'no class', keys: [], readers: [] };
	const keys = [...new Set([...cb.body.matchAll(/"([a-z][a-z0-9_]*\.[a-z0-9_.]+)"/g)].map(m => m[1]))];
	if (keys.length) {
		const readers = [...new Set(keys.flatMap(k => files.filter(f => !registration.test(f.p) && f.s.includes('"' + k + '"')).map(f => f.p)))];
		return { how: 'tune', keys, readers, file: cb.file };
	}
	const wrapped = cb.body.match(/return SCR_BaseEditorAttributeVar\.Create[A-Za-z]*\(\s*!?[A-Za-z_]+\.([A-Za-z0-9_]+)\(/);
	const plain = cb.body.match(/(?:Get|Read)[A-Za-z]*\([^)]*\)\s*\{[^}]*?return (?!SCR_BaseEditorAttributeVar\.)[A-Za-z_]+\.([A-Za-z0-9_]+)\(/);
	const g = (wrapped || plain || [])[1];
	if (!g) return { how: 'unresolved', keys: [], readers: [], file: cb.file };
	let field = null;
	for (const x of files) { const m = x.s.match(new RegExp(B + g + '[(][^)]*[)]\\s*[{]\\s*return (m_[A-Za-z0-9_]+);')); if (m) { field = m[1]; break; } }
	const tokens = field ? [g, field] : [g];
	const readers = [...new Set(tokens.flatMap(readersOf))];
	return { how: field ? 'field' : 'getter', keys: tokens, readers, file: cb.file };
}

function declaredFull() {
	if (!fs.existsSync(DOC)) return new Set();
	const full = new Set();
	for (const l of fs.readFileSync(DOC, 'utf8').split('\n')) {
		const c = l.split('|').map(s => s.trim());
		if (c.length > 6 && /^full/i.test(c[6])) full.add(c[2].replace(/`/g, ''));
	}
	return full;
}

const full = declaredFull();
const rows = [];
for (const a of attributes()) {
	const r = resolve(a.cls);
	let status = 'not experimental', why = 'committed, unchanged';
	const newKeys = r.how === 'tune' ? r.keys.filter(k => !baseC.includes('"' + k + '"')) : [];
	const touched = r.readers.filter(p => changed.has(p));
	if (!baseClasses.has(a.cls)) { status = 'experimental'; why = 'new setting'; }
	else if (newKeys.length) { status = 'experimental'; why = 'new key ' + newKeys.join(', '); }
	else if (touched.length) { status = 'experimental'; why = 'changed: ' + touched.map(p => path.basename(p)).slice(0, 4).join(', ') + (touched.length > 4 ? ' +' + (touched.length - 4) : ''); }
	else if (!r.readers.length) { status = 'borderline'; why = r.how === 'unresolved' || r.how === 'no class' ? 'key not found in class ' + (r.file ? path.basename(r.file) : '-') : 'no reader outside GM/Settings for ' + r.keys.join(', '); }
	if (full.has(a.cls)) status = 'full (user)';
	rows.push({ ...a, keys: r.keys, status, why });
}

const beta = new Set(rows.filter(r => r.status === 'experimental').map(r => r.cls));

const help = fs.readFileSync(HELP, 'utf8');
const nl = help.includes('\r\n') ? '\r\n' : '\n';
const lines = help.split(/\r?\n/).filter(l => l !== '   m_iBeta 1');
const outLines = [];
let marked = 0;
for (const l of lines) {
	outLines.push(l);
	const m = l.match(/^   m_sAttribute "(.*)"$/);
	if (m && beta.has(m[1])) { outLines.push('   m_iBeta 1'); marked++; }
}
const helpOut = outLines.join(nl);

const cell = s => String(s).replace(/\|/g, '/');
const counts = rows.reduce((o, r) => (o[r.status] = (o[r.status] || 0) + 1, o), {});
let md = '# DCO Experimental Features (GM Settings)\n\n';
md += 'Generated by `node tools/dco-experimental.js` against git ' + BASE + ' (' + git('log -1 --format=%h ' + BASE).trim() + '). Do not edit by hand except the status column.\n\n';
md += 'Rule (user 2026-10-11): experimental = not yet declared "fully done" by the user AND new or changed since the last commit. ';
md += 'To clear a setting: set its status to `full` here and re-run the generator; its m_iBeta flag in DCO_SettingsHelp.conf is removed.\n\n';
md += 'Totals: ' + Object.entries(counts).map(([k, v]) => k + ' ' + v).join(', ') + '. Flags written: ' + marked + '.\n\n';
md += '| setting | class | key | why | category | status |\n|---|---|---|---|---|---|\n';
const order = { 'experimental': 0, 'borderline': 1, 'full (user)': 2, 'not experimental': 3 };
rows.sort((x, y) => order[x.status] - order[y.status] || x.cat.localeCompare(y.cat) || x.name.localeCompare(y.name));
for (const r of rows) md += '| ' + [cell(r.name), '`' + r.cls + '`', cell(r.keys.join(', ') || '-'), cell(r.why), r.cat, r.status].join(' | ') + ' |\n';

const missingHelp = [...beta].filter(c => !new RegExp('m_sAttribute "' + c + '"').test(help));
console.log('attributes ' + rows.length + ' | ' + Object.entries(counts).map(([k, v]) => k + ' ' + v).join(' | ') + ' | flags ' + marked + (missingHelp.length ? ' | no help entry: ' + missingHelp.join(',') : ''));
if (!DRY) {
	fs.writeFileSync(DOC, md);
	if (!DOC_ONLY && helpOut !== help) fs.writeFileSync(HELP, helpOut);
	console.log('wrote ' + rel(DOC) + (DOC_ONLY ? '' : ' and ' + rel(HELP)));
}
