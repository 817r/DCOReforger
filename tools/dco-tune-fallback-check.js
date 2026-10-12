const fs=require('fs'),path=require('path');
const root='D:/Git/DCOReforger/DCO_AI/scripts/Game';
const files=[];(function w(d){for(const e of fs.readdirSync(d,{withFileTypes:true})){const p=path.join(d,e.name);if(e.isDirectory())w(p);else if(p.endsWith('.c'))files.push(p);}})(root);
const def={};const uses=[];
const num=s=>{s=s.trim().replace(/f$/,'');return /^-?[0-9.]+$/.test(s)?parseFloat(s):null;};
for(const f of files){const t=fs.readFileSync(f,'utf8');const L=t.split('\n');
 L.forEach((l,i)=>{let m;const re1=/DCO_TuneCfg\("([^"]+)",\s*"[^"]+",\s*([^,]+),/g;while((m=re1.exec(l)))def[m[1]]={v:num(m[2]),at:path.relative(root,f)+':'+(i+1)};
 const re2=/"([a-z]+\.[A-Za-z0-9_.]+)",\s*(-?[0-9.]+f?)\s*[,)]/g;while((m=re2.exec(l))){if(/DCO_TuneCfg/.test(l))continue;uses.push({k:m[1],v:num(m[2]),at:path.relative(root,f)+':'+(i+1)});}});}
let bad=0;const seen=new Set();
for(const u of uses){const d=def[u.k];if(!d||u.v===null||d.v===null)continue;if(Math.abs(d.v-u.v)>1e-9){bad++;const key=u.k+'|'+u.v;if(seen.has(key))continue;seen.add(key);console.log(u.k,'cfg='+d.v,'callsite='+u.v,u.at,'(cfg '+d.at+')');}}
console.log('keys',Object.keys(def).length,'uses',uses.length,'mismatch',bad);
