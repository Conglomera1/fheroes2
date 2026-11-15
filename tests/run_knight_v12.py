"""Opening sea crews, ordinary population retention and troop conservation."""
import json, os, pathlib, subprocess, sys, tempfile, collections
binary=pathlib.Path(sys.argv[1]).resolve(); out=pathlib.Path(sys.argv[2]).resolve(); out.mkdir(parents=True,exist_ok=True)
cases=['cargo_reinforce','cargo_deliver','cargo_captured','cargo_timeout','cargo_danger','cargo_meeting','cargo_arrive','naval','naval_origin','naval_poor','naval_late','naval_danger','naval_no_goal','cash_income','cash_cooldown','defense_well','defense_farm','adventure','adventure_emergency']
profiles=[('default','# Default\n',cases)]
records=[]
def army(values):
    c=collections.Counter()
    for u,n in values:
        if u:c[u]+=n
    return c
for profile,ini,modes in profiles:
    for mode in modes:
        pair=[]
        for level in [0,2]:
            with tempfile.TemporaryDirectory(prefix='km-v12-') as d:
                root=pathlib.Path(d);(root/'fheroes2').mkdir();(root/'fheroes2/knight_mod.ini').write_text(ini)
                cfg=root/'settings.cfg';cfg.write_text(f'taxpayer test diagnostics = {level}\n')
                p=subprocess.run([str(binary),mode,str(cfg)],env=dict(os.environ,XDG_CONFIG_HOME=d),capture_output=True,text=True,timeout=50)
                log=p.stdout+p.stderr;(out/f'{profile}-{mode}-d{level}.log').write_text(log)
                assert p.returncode==0,(profile,mode,level,log[-4000:])
                result='\n'.join(line for line in log.splitlines() if line.startswith('PASS v12'))
                assert result
                if not level:assert '[KNIGHT ECONOMY]' not in log
                for line in log.splitlines():
                    if '[KNIGHT ECONOMY] ' not in line:continue
                    r=json.loads(line.split('[KNIGHT ECONOMY] ',1)[1])
                    if r['event']=='OBSERVER_ARMY_TRANSFER':
                        assert army(r['source_before'])+army(r['destination_before'])==army(r['source_after'])+army(r['destination_after']),r
                pair.append(result);records.append(dict(profile=profile,case=mode,diagnostics=level,result=result))
        assert pair[0]==pair[1],pair
        print(profile,pair[0],flush=True)
(out/'results.json').write_text(json.dumps(records,indent=2)+'\n')
print(f'PASS {len(records)} v12 processes, diagnostics neutral')
