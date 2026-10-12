// Finds DCO log lines that print a raw object pointer ("<0x...>") instead of a readable id.
// Usage: node tools/dco-logptr-check.js [--sessions=N] [--logs=<DCO Logs dir>]
// Exit 1 when any event prints a pointer; the fix is DCO_Log.IdGroup()/IdUnit() in the string.Format.
const fs = require('fs'), path = require('path');
const arg = n => (process.argv.find(a => a.startsWith('--' + n + '=')) || '').split('=')[1];
const LOGS = arg('logs') || path.join(process.env.USERPROFILE || '', 'OneDrive/文档/My Games/ArmaReforgerWorkbench/profile/DCO/Logs');
const N = +(arg('sessions') || 20);
const sessions = [];
const dirs = p => fs.existsSync(p) ? fs.readdirSync(p, { withFileTypes: true }).filter(e => e.isDirectory()).map(e => e.name) : [];
for (const day of dirs(LOGS))
  for (const b of dirs(path.join(LOGS, day)))
    for (const s of dirs(path.join(LOGS, day, b))) sessions.push(path.join(LOGS, day, b, s));
sessions.sort((a, b) => (path.basename(path.dirname(path.dirname(a))) + path.basename(a)).localeCompare(path.basename(path.dirname(path.dirname(b))) + path.basename(b)));
const recent = sessions.slice(-N);
const counts = {};
for (const d of recent) {
  const f = path.join(d, 'dco.log');
  if (!fs.existsSync(f)) continue;
  for (const l of fs.readFileSync(f, 'utf8').split('\n')) {
    if (!l.includes('<0x')) continue;
    const m = l.match(/ [A-Z] [A-Z]+ \S+ ([A-Za-z_]+)[ =]/);
    const fld = (l.match(/ ([a-z_]+)=<0x/) || [, '?'])[1];
    const k = (m ? m[1] : '?') + ' ' + fld + '=';
    counts[k] = (counts[k] || 0) + 1;
  }
}
const rows = Object.entries(counts).sort((a, b) => b[1] - a[1]);
console.log('dco-logptr-check: ' + recent.length + ' sessions, ' + rows.length + ' event(s) printing raw pointers');
for (const [k, n] of rows) console.log('  ' + n + '  ' + k);
process.exit(rows.length ? 1 : 0);
