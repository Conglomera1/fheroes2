"""Link the real construction adapter against a matching native debug build."""
from pathlib import Path
import subprocess,sys
root=Path(__file__).resolve().parents[1]
incs=['-I'+str(p) for p in (root/'src/fheroes2').iterdir() if p.is_dir()]+['-I'+str(root/'src/engine'),'-I'+str(root/'src/thirdparty/libsmacker')]
obj=Path(sys.argv[1]);binary=Path(sys.argv[2]);sdl=Path(sys.argv[3]);mixer=Path(sys.argv[4])
subprocess.run(['g++','-std=c++17','-O0','-g','-DWITH_DEBUG','-DFHEROES2_TAXPAYER_DIAGNOSTICS',*incs,'-I'+str(sdl),'-c',str(root/(sys.argv[5] if len(sys.argv)>5 else 'tests/knight_construction_engine_test.cpp')),'-o',str(obj)],check=True)
game=sorted(p for p in (root/'src/dist/fheroes2').glob('*.o') if p.name not in {'ai_planner_castle.o','fheroes2.o'})
subprocess.run(['g++','-o',str(binary),str(obj),*map(str,game),str(root/'src/dist/engine/libengine.a'),str(root/'src/dist/thirdparty/libsmacker/libsmacker.a'),str(mixer),'/lib/x86_64-linux-gnu/libSDL2-2.0.so.0','-pthread','-lz'],check=True)
