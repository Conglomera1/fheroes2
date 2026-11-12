"""Production regressions; diagnostics must not change purchases or decisions."""
import json, os, pathlib, subprocess, sys, tempfile
binary = pathlib.Path(sys.argv[1]).resolve()
out = pathlib.Path(sys.argv[2]).resolve(); out.mkdir(parents=True, exist_ok=True)
cases = [f'history_{n}' for n in range(4)] + ['optional_gold', 'same_day', 'existing', 'duplicate', 'defender', 'other_defender', 'spent', 'sleeping', 'hidden', 'shipyard_future', 'summon']
results = []
for case in cases:
    pair = []
    for level in [0, 2]:
        with tempfile.TemporaryDirectory(prefix='km-v8-') as d:
            (pathlib.Path(d) / 'fheroes2').mkdir()
            (pathlib.Path(d) / 'fheroes2/knight_mod.ini').write_text('# Default\n')
            cfg = pathlib.Path(d) / 'settings.cfg'; cfg.write_text(f'taxpayer test diagnostics = {level}\n')
            p = subprocess.run([str(binary), case, str(cfg)], env=dict(os.environ, XDG_CONFIG_HOME=d), capture_output=True, text=True, timeout=40)
            log = p.stdout + p.stderr; (out / f'{case}-d{level}.log').write_text(log)
            assert p.returncode == 0, (case, level, log[-3500:])
            value = '\n'.join(line for line in log.splitlines() if line.startswith(('PASS v8', 'FORECAST ')))
            if level == 0: assert '[KNIGHT ECONOMY]' not in log
            else:
                for line in log.splitlines():
                    if '[KNIGHT ECONOMY] ' in line: json.loads(line.split('[KNIGHT ECONOMY] ', 1)[1])
            pair.append(value); results.append(dict(case=case, diagnostics=level, result=value))
    assert pair[0] == pair[1], pair
    print(pair[0], flush=True)
(out / 'results.json').write_text(json.dumps(results, indent=2) + '\n')
print(f'PASS {len(results)} v8 processes, diagnostics neutral')
