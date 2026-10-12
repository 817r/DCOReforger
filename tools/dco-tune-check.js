const fs=require('fs'),path=require('path');
const root='D:/Git/DCOReforger/DCO_AI/scripts';
function walk(d,o){for(const e of fs.readdirSync(d,{withFileTypes:true})){const p=path.join(d,e.name);if(e.isDirectory())walk(p,o);else if(p.endsWith('.c'))o.push(p)}return o}
const reg={},reads={};
for(const f of walk(root,[])){const rel=path.relative(root,f).split(path.sep).join('/');const lines=fs.readFileSync(f,'utf8').split('\n');
 lines.forEach((l,i)=>{let m=l.match(/DCO_TuneCfg\("([^"]+)",\s*"[^"]+",\s*([-\d.]+)/);if(m){reg[m[1]]={def:+m[2],at:rel+':'+(i+1)};return}
  for(const r of l.matchAll(/(?:DCO_TuneF|TuneG|TuneFor|TuneForU|TuneForGU|TuneForCmd|DCO_Stress\.T|\bT)\((?:[^()"]*?,\s*)?"([a-z0-9_]+\.[A-Za-z0-9_]+)",\s*([-\d.]+)\s*\)/g)){(reads[r[1]]=reads[r[1]]||[]).push({fb:+r[2],at:rel+':'+(i+1)})}
 });}
let bad=0;
for(const [k,v] of Object.entries(reg)){const rs=reads[k]||[];const mism=rs.filter(r=>Math.abs(r.fb-v.def)>1e-6);if(mism.length){bad++;console.log(`${k}: registered default ${v.def} (${v.at}); readers with different fallback: ${mism.map(r=>r.fb+' @'+r.at).join(', ')}`)}}
console.log('dco-tune-check: registered',Object.keys(reg).length,'keys; mismatched fallback',bad);process.exit(bad?1:0);
