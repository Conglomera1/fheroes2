"""Build a native Knight fixture with drawing/input-only link wrappers."""
import argparse, pathlib, subprocess, sys
p=argparse.ArgumentParser();p.add_argument('--source',required=True);p.add_argument('--output',required=True);a=p.parse_args()
r=pathlib.Path(__file__).resolve().parents[1];n=r.parent/'native-check';n.mkdir(exist_ok=True);render=pathlib.Path(a.output).resolve().with_suffix('.render.o')
subprocess.run(['g++','-c',str(r/'tests/knight_v15_headless_render.cpp'),'-o',str(render)],check=True)
wraps=['_ZN10LocalEvent12HandleEventsEbb','_ZN9Interface11StatusPanel18drawAITurnProgressEj','_ZN8fheroes223generateScrollbarSliderERKNS_5ImageEbiiiRKNS_10RectBase2DIiiEES6_','_ZNK9Interface8GameArea6RedrawERN8fheroes25ImageEib','_ZNK9Interface11StatusPanel7_redrawEv']
flags=' '.join('-Wl,--wrap='+x for x in wraps)+' '+str(render)
subprocess.run([sys.executable,str(r/'tests/build_exact_recruit_adapter.py'),'--output',a.output,'--source',a.source,'--exclude-object','ai_planner_castle.o','--sdl-flags=-I'+str(r.parent/'sdl2-headers/include')+' -I'+str(r.parent/'sdl2-mixer-headers/include'),'--sdl-libs='+str(n/'libSDL2_mixer.so')+' /lib/x86_64-linux-gnu/libSDL2-2.0.so.0','--ldflags='+flags],check=True)
