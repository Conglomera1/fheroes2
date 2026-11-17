import pathlib,tempfile,subprocess,os,sys,json
binary=str(pathlib.Path(sys.argv[1]).resolve());out=pathlib.Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
cases={'default':'','legacy':'taxpayer test reveal map = on\ntaxpayer test diagnostics = 2\n','observer':'knight observer = on\n','diagnostics':'knight diagnostics = 1\n','both':'knight observer = on\nknight diagnostics = 1\n','clamped':'knight diagnostics = 8\n','negative':'knight diagnostics = -2\n'}
records=[]
for mode,cfg in cases.items():
 with tempfile.TemporaryDirectory() as d:
  p=pathlib.Path(d)/'settings.cfg';p.write_text(cfg);r=subprocess.run([binary,mode,str(p)],capture_output=True,text=True,env=dict(os.environ,XDG_CONFIG_HOME=d),timeout=30);log=r.stdout+r.stderr;(out/(mode+'.log')).write_text(log);assert r.returncode==0,(mode,log[-3000:]);line=next(x for x in log.splitlines() if x.startswith('PASS release_settings'));print(line,flush=True);records.append(dict(case=mode,result=line))
(out/'results.json').write_text(json.dumps(records,indent=2)+'\n')
