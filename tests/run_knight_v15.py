"""Opening sea crews, ordinary population retention and troop conservation."""
import json, os, pathlib, subprocess, sys, tempfile, collections
binary=pathlib.Path(sys.argv[1]).resolve(); out=pathlib.Path(sys.argv[2]).resolve(); out.mkdir(parents=True,exist_ok=True)
cases=['opening_coast','route_wait_clear','route_wait_timeout','route_wait_danger','opening_aow','opening_cox','opening_poor','opening_no_goal','opening_late','opening_human','opening_danger','opening_cox_no_high',
 'peasants_weak','peasants_sufficient','supply_peasant_pickup','supply_utopia_j192','supply_hunter','supply_sleep','supply_direct_full','supply_unsafe','supply_occupied','supply_fighter','growth_plague']
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
            with tempfile.TemporaryDirectory(prefix='km-v15-') as d:
                root=pathlib.Path(d);(root/'fheroes2').mkdir();(root/'fheroes2/knight_mod.ini').write_text(ini)
                cfg=root/'settings.cfg';cfg.write_text(f'taxpayer test diagnostics = {level}\n')
                p=subprocess.run([str(binary),mode,str(cfg)],env=dict(os.environ,XDG_CONFIG_HOME=d),capture_output=True,text=True,timeout=50)
                log=p.stdout+p.stderr;(out/f'{profile}-{mode}-d{level}.log').write_text(log)
                assert p.returncode==0,(profile,mode,level,log[-4000:])
                result='\n'.join(line for line in log.splitlines() if line.startswith('PASS v15'))
                assert result
                if not level:assert '[KNIGHT ECONOMY]' not in log
                for line in log.splitlines():
                    if '[KNIGHT ECONOMY] ' not in line:continue
                    r=json.loads(line.split('[KNIGHT ECONOMY] ',1)[1])
                    if r['event']=='NAVAL_OPENING_PARTY':
                        assert army(r['land_before'])+army(r['sea_before'])+army(r['shore_before'])==army(r['land_after'])+army(r['sea_after'])+army(r['shore_after']),r
                    if r['event']=='OBSERVER_ARMY_TRANSFER':
                        assert army(r['source_before'])+army(r['destination_before'])==army(r['source_after'])+army(r['destination_after']),r
                pair.append(result);records.append(dict(profile=profile,case=mode,diagnostics=level,result=result))
        assert pair[0]==pair[1],pair
        print(profile,pair[0],flush=True)
(out/'results.json').write_text(json.dumps(records,indent=2)+'\n')
print(f'PASS {len(records)} v15 processes, diagnostics neutral')
