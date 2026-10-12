const fs = require('fs');
const path = require('path');

const ROOT = process.argv[2] || path.join(__dirname, '..', 'DCO_AI');
const SCRIPTS = path.join(ROOT, 'scripts');
const B = '(?<![A-Za-z0-9_])';
const W = '(?![A-Za-z0-9_])';

function walk(dir, out = []) {
	for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
		const p = path.join(dir, e.name);
		if (e.isDirectory()) walk(p, out);
		else if (p.endsWith('.c')) out.push(p);
	}
	return out;
}
const files = walk(SCRIPTS).map(p => ({ p: p.replace(/\\/g, '/'), s: fs.readFileSync(p, 'utf8') }));
const read = p => fs.readFileSync(path.join(ROOT, p), 'utf8').replace(/\r\n/g, '\n');

function editAttributes() {
	const out = [];
	let cur = null;
	for (const l of read('Configs/Editor/AttributeLists/Edit.conf').split('\n')) {
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

function isDeclaration(l) {
	return !/^\s*return\s/.test(l) && /^\s*(protected |private |static )*(ref )?[A-Za-z<>]+ m_[A-Za-z0-9_]+(\s*=.*)?;\s*$/.test(l) || /^\s*\[Attribute/.test(l);
}

function fieldIsRead(field, skip) {
	const fre = new RegExp(B + field + W);
	const getters = new Set();
	for (const f of files) for (const x of f.s.matchAll(new RegExp('([A-Za-z_][A-Za-z0-9_]*)[(][^)]*[)]\\s*[{]\\s*return ' + field + '\\s*;', 'g'))) getters.add(x[1]);
	for (const f of files) {
		if (skip.test(f.p)) continue;
		for (const l of f.s.split('\n')) {
			if (fre.test(l)) {
				if (isDeclaration(l) || /Cfg[FIBS]\(|DCO_P[FBIS]\(/.test(l)) continue;
				if (new RegExp('return ' + field + '\\s*;\\s*[}]?\\s*$').test(l) && /[{]/.test(l)) continue;
				const rest = l.replace(new RegExp(field + '\\s*=(?!=)'), '');
				if (fre.test(rest)) return true;
				continue;
			}
			for (const g of getters) if (new RegExp(B + g + '[(]').test(l) && !new RegExp(g + '[(][^)]*[)]\\s*[{]').test(l)) return true;
		}
	}
	return false;
}

const attrs = editAttributes();
const attrSet = new Set(attrs.map(a => a.cls));
const problems = { noClass: [], noHelp: [], helpOrphan: [], deadAttr: [], deadConfig: [], deadTune: [], dupName: [] };

for (const a of attrs) if (!files.some(f => new RegExp('class ' + a.cls + W).test(f.s))) problems.noClass.push(a.cls);

const help = read('Configs/DCO/DCO_SettingsHelp.conf');
const helpAttrs = new Set([...help.matchAll(/m_sAttribute "(.*?)"/g)].map(m => m[1]));
for (const a of attrs) if (!helpAttrs.has(a.cls)) problems.noHelp.push(a.cls);
for (const h of helpAttrs) if (!attrSet.has(h)) problems.helpOrphan.push(h);

const seen = {};
for (const a of attrs) { const k = a.cat + '|' + a.name.toLowerCase(); (seen[k] = seen[k] || []).push(a.cls); }
for (const [k, v] of Object.entries(seen)) if (v.length > 1) problems.dupName.push(k + ': ' + v.join(','));

const skipGm = /\/GM\/|\/Settings\/|TuneAttributes|CommanderPresets|Serializer/;
for (const a of attrs) {
	const f = files.find(x => new RegExp('class ' + a.cls + W).test(x.s));
	if (!f) continue;
	const i = f.s.search(new RegExp('class ' + a.cls + W));
	const blk = f.s.slice(i, i + 2500).split(/\nclass |\n\[BaseContainerProps/)[0];
	const g = (blk.match(/(?:Get|ReadVariable)[^{]*\{[^}]*?return [A-Za-z_]+\.([A-Za-z0-9_]+)\(\)/) || [])[1];
	if (!g) continue;
	let field = null;
	for (const x of files) { const m = x.s.match(new RegExp(B + g + '[(][)]\\s*[{]\\s*return (m_[A-Za-z0-9_]+);')); if (m) { field = m[1]; break; } }
	if (field && !fieldIsRead(field, skipGm)) problems.deadAttr.push(a.cls + ' (' + a.name + ') -> ' + field);
}

for (const f of files) {
	for (const m of f.s.matchAll(/(m_[A-Za-z0-9_]+)\s*=\s*Cfg[FIBS]\("([^"]+)",\s*\1\b/g)) {
		if (m[2] === 'aimModel') continue;
		if (!fieldIsRead(m[1], /\/GM\/|TuneAttributes|CommanderPresets|Serializer/)) problems.deadConfig.push(m[2] + ' -> ' + m[1]);
	}
}

const gt = files.find(f => f.p.endsWith('Global/DCO_GlobalTune.c'));
if (gt) {
	const nonTune = files.filter(f => !/Tune\.c$|TuneAttributes|GlobalTune/.test(f.p));
	for (const m of gt.s.matchAll(/DCO_TuneCfg\("([^"]+)",\s*"([^"]+)"/g)) {
		const key = m[1];
		if (nonTune.some(f => f.s.includes('"' + key + '"'))) continue;
		const field = key.split('.')[1];
		const fre = new RegExp(B + 'm_[a-z]{1,2}(' + field + '|' + field.replace(/Scale$/, '') + ')(_[A-Z]{2,3})?' + W);
		const used = nonTune.some(f => f.s.split('\n').some(l => fre.test(l) && !isDeclaration(l)));
		if (!used) problems.deadTune.push(key + ' (' + m[2] + ')');
	}
}

let total = 0;
console.log('GM attributes: ' + attrs.length + ', help entries: ' + helpAttrs.size);
for (const [k, v] of Object.entries(problems)) {
	total += v.length;
	console.log('\n[' + k + '] ' + v.length);
	for (const x of v) console.log('  ' + x);
}
process.exit(total ? 1 : 0);
