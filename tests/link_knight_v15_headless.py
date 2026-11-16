"""Relink an already compiled v15 fixture; replace drawing only, never AI/world logic."""
from pathlib import Path
import subprocess,sys
r=Path(__file__).resolve().parents[1];n=r.parent/'native-check'
subprocess.run(['g++','-c',str(r/'tests/knight_v15_headless_render.cpp'),'-o',str(n/'v15-headless-render.o')],check=True)
objects=sorted(str(p) for p in (r/'src/dist/fheroes2').glob('*.o') if p.name not in ['fheroes2.o','ai_planner_castle.o'])
wraps=['_ZN10LocalEvent12HandleEventsEbb','_ZN9Interface11StatusPanel18drawAITurnProgressEj','_ZN8fheroes223generateScrollbarSliderERKNS_5ImageEbiiiRKNS_10RectBase2DIiiEES6_','_ZNK9Interface8GameArea6RedrawERN8fheroes25ImageEib','_ZNK9Interface11StatusPanel7_redrawEv']
subprocess.run(['g++','-o',str(n/'v15-full-tests'),str(n/'v15-full-tests.o'),str(n/'v15-headless-render.o'),*objects,str(r/'src/dist/engine/libengine.a'),str(r/'src/dist/thirdparty/libsmacker/libsmacker.a'),'-pthread','-lz',str(n/'libSDL2_mixer.so'),'/lib/x86_64-linux-gnu/libSDL2-2.0.so.0',*['-Wl,--wrap='+x for x in wraps]],check=True)
