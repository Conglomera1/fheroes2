"""Production regressions across turns; diagnostics must not affect choices."""
import json, os, pathlib, subprocess, sys, tempfile
binary = pathlib.Path(sys.argv[1]).resolve()
out = pathlib.Path(sys.argv[2]).resolve(); out.mkdir(parents=True, exist_ok=True)
cases = ['voyage', 'land_goal', 'sea_defense', 'reward_first', 'sea_only', 'trade_reject', 'trade_allow', 'trade_emergency', 'trade_log', 'direct_secondary', 'entry_funding', 'small_gain', 'sustained', 'recovery_reentry']
records = []
for case in cases:
    pair = []
    for level in [0, 2]:
        with tempfile.TemporaryDirectory(prefix='km-v9-') as d:
            root = pathlib.Path(d); (root / 'fheroes2').mkdir(); (root / 'fheroes2/knight_mod.ini').write_text('# Default\n')
            cfg = root / 'settings.cfg'; cfg.write_text(f'taxpayer test diagnostics = {level}\n')
            p = subprocess.run([str(binary), case, str(cfg)], env=dict(os.environ, XDG_CONFIG_HOME=d), capture_output=True, text=True, timeout=50)
            log = p.stdout + p.stderr; (out / f'{case}-d{level}.log').write_text(log)
            assert p.returncode == 0, (case, level, log[-4000:])
            result = '\n'.join(line for line in log.splitlines() if line.startswith(('PASS v9', 'DECISION ', 'TRADE ')))
            if level == 0: assert '[KNIGHT ECONOMY]' not in log
            for line in log.splitlines():
                if '[KNIGHT ECONOMY] ' in line: json.loads(line.split('[KNIGHT ECONOMY] ', 1)[1])
            pair.append(result); records.append(dict(case=case, diagnostics=level, result=result))
    assert pair[0] == pair[1], pair
    print(pair[0], flush=True)
(out / 'results.json').write_text(json.dumps(records, indent=2) + '\n')
print(f'PASS {len(records)} v9 processes, diagnostics neutral')
