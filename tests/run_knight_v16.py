"""Opening sea crews, ordinary population retention and troop conservation."""
import json, os, pathlib, subprocess, sys, tempfile, collections
binary=pathlib.Path(sys.argv[1]).resolve(); out=pathlib.Path(sys.argv[2]).resolve(); out.mkdir(parents=True,exist_ok=True)
cases=['control','delivery','far_occupied','full_turn','loading_blocked','danger','military']
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
            with tempfile.TemporaryDirectory(prefix='km-v16-') as d:
                root=pathlib.Path(d);(root/'fheroes2').mkdir();(root/'fheroes2/knight_mod.ini').write_text(ini)
                cfg=root/'settings.cfg';cfg.write_text(f'taxpayer test diagnostics = {level}\n')
                p=subprocess.run([str(binary),mode,str(cfg)],env=dict(os.environ,XDG_CONFIG_HOME=d),capture_output=True,text=True,timeout=50)
                log=p.stdout+p.stderr;(out/f'{profile}-{mode}-d{level}.log').write_text(log)
                assert p.returncode==0,(profile,mode,level,log[-4000:])
                result='\n'.join(line for line in log.splitlines() if line.startswith('PASS v16'))
                assert result
                if not level:assert '[KNIGHT ECONOMY]' not in log
                for line in log.splitlines():
                    if '[KNIGHT ECONOMY] ' not in line:continue
                    r=json.loads(line.split('[KNIGHT ECONOMY] ',1)[1])
                    if r['event']=='NAVAL_OPENING_PARTY':
                        assert army(r['land_before'])+army(r['sea_before'])+army(r['shore_before'])==army(r['land_after'])+army(r['sea_after'])+army(r['shore_after']),r
                    if r['event']=='OBSERVER_ARMY_TRANSFER':
                        assert army(r['source_before'])+army(r['destination_before'])==army(r['source_after'])+army(r['destination_after']),r
                if mode in ['delivery','far_occupied','full_turn','control'] and level:
                    trace=[json.loads(line.split('[KNIGHT ECONOMY] ',1)[1]) for line in log.splitlines() if '[KNIGHT ECONOMY] ' in line]
                    cons=[r for r in trace if r['event']=='CONSCRIPTION_DAY' and r['town']==5678]
                    assert sum(r['success_total_after']-r['success_total_before'] for r in cons)==(7 if mode=='control' else 10)
                    plans=[r for r in trace if r['event']=='POPULATION_SUPPLY_PLAN']
                    if mode=='control':assert not plans
                    else:
                        assert len(plans)==1 and plans[0]['hero']==14 and plans[0]['quantity']==6,plans
                        assert plans[0]['pickup_cost']==400 and 2000<plans[0]['delivery_cost']<=4000
                        arrival=[r for r in trace if r['event']=='KNIGHT_POPULATION' and r['town']==5678 and r['hero']==14 and r['carried']==0 and r['garrison']>=6]
                        assert arrival and min(r['day'] for r in arrival)<=145
                        assert not any(r.get('reason') in ['safe_fallback','delivery_timeout','destination_occupied'] for r in trace)
                pair.append(result);records.append(dict(profile=profile,case=mode,diagnostics=level,result=result))
        assert pair[0]==pair[1],pair
        print(profile,pair[0],flush=True)
(out/'results.json').write_text(json.dumps(records,indent=2)+'\n')
print(f'PASS {len(records)} v16 processes, diagnostics neutral')
