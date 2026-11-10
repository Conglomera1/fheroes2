"""Record real API observations, retaining inconsistent raw results as RED/exit 2."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

parser=argparse.ArgumentParser()
parser.add_argument('binary',type=Path)
parser.add_argument('--output-dir',type=Path,required=True)
args=parser.parse_args(); exe=args.binary.resolve(); output=args.output_dir.resolve(); output.mkdir(parents=True,exist_ok=True)
modes=('raw_exact','raw_overstock','guarded_overstock','raw_empty','guarded_empty','raw_zero','poor','full','visitor')
records=[]
for mode in modes:
    baseline=None
    for level in (0,2):
        with tempfile.TemporaryDirectory(prefix='km-receipt-') as d:
            root=Path(d); (root/'fheroes2').mkdir(); (root/'fheroes2/knight_mod.ini').write_text('# Defaults\n')
            cfg=root/'settings.cfg'; cfg.write_text(f'taxpayer test diagnostics = {level}\n')
            p=subprocess.run([str(exe),str(cfg),mode],env=dict(os.environ,XDG_CONFIG_HOME=d),stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=40)
            (output/f'{mode}-d{level}.log').write_text(f'RETURN_CODE={p.returncode}\n'+p.stdout)
            assert p.returncode in (0,2),p.stdout
            result=next(l for l in p.stdout.splitlines() if l.startswith('RESULT '))
            values=dict(field.split('=',1) for field in result.split()[1:])
            observed={key:(value if key=='mode' else int(value)) for key,value in values.items()}
            assert (p.returncode==0)==bool(observed['receipt_consistent']),observed
            if mode not in ('raw_overstock','raw_empty'):
                assert p.returncode==0,observed
            inside=p.stdout.split('PROBE_BEGIN\n',1)[1].split('PROBE_END\n',1)[0]
            rows=[json.loads(l.split('[KNIGHT ECONOMY] ',1)[1]) for l in inside.splitlines() if '[KNIGHT ECONOMY] ' in l]
            if level==0:
                assert not rows
            else:
                purchases=[r for r in rows if r['event']=='TROOP_PURCHASE']
                assert len(purchases)==observed['accepted']
                if purchases:
                    assert purchases[0]['kind']=='actual' and purchases[0]['requested_count']==observed['requested']
                    assert purchases[0]['stock_removed']==observed['stock_removed']
            if baseline is None: baseline=observed
            else: assert observed==baseline,(observed,baseline)
            record=dict(observed,diagnostics=level,exit_code=p.returncode,classification='RED_API_INCONSISTENCY' if p.returncode==2 else 'CONSISTENT_CONTROL')
            records.append(record)
            (output/'observations.json').write_text(json.dumps(records,indent=2)+'\n')
    print(f"{'RED' if not baseline['receipt_consistent'] else 'CONTROL'} {mode}: requested={baseline['requested']} stock={baseline['stock_before']} added={baseline['units_added']} removed={baseline['stock_removed']} gold={baseline['gold_spent']}; diagnostics neutral",flush=True)
red=sum(r['exit_code']==2 for r in records)
print(f'Completed {len(records)} native observations: {len(records)-red} consistent controls, {red} RED API results (raw exit codes retained; no API fix).')
