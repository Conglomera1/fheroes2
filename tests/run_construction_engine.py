"""Run actual construction cases with diagnostics off/on; preserve logs and results."""
import json,os,pathlib,subprocess,sys,tempfile
binary=pathlib.Path(sys.argv[1]).resolve();out=pathlib.Path(sys.argv[2]).resolve();out.mkdir(parents=True,exist_ok=True)
records=[]
for mode in ['statue','hut','archer','well','growth','farm_1000','wait','shared','futurewood','castle_income']:
    pair=[]
    for level in [0,2]:
        with tempfile.TemporaryDirectory(prefix='km-construction-') as d:
            cfg=pathlib.Path(d)/'settings.cfg';cfg.write_text(f'taxpayer test diagnostics = {level}\n')
            p=subprocess.run([str(binary),mode,str(cfg)],env=dict(os.environ,XDG_CONFIG_HOME=d),capture_output=True,text=True,timeout=30)
            log=p.stdout+p.stderr;(out/f'{mode}-d{level}.log').write_text(log)
            assert p.returncode==0,(mode,level,log[-2000:])
            result=next(line for line in log.splitlines() if line.startswith('PASS real construction'))
            if level==0:assert '[KNIGHT ECONOMY]' not in log
            pair.append(result);records.append({'case':mode,'diagnostics':level,'result':result,'exit_code':p.returncode})
    assert pair[0]==pair[1],(mode,pair)
(out/'results.json').write_text(json.dumps(records,indent=2))
print(f'PASS {len(records)} real-engine construction runs; diagnostics neutral')
