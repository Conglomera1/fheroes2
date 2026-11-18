"""Verify a downloaded public beta ZIP against its source and generated catalogs."""
import argparse, gettext, hashlib, io, json, pathlib, struct, zipfile
p=argparse.ArgumentParser();p.add_argument('archive',type=pathlib.Path);p.add_argument('--arch',choices=['x86','x64'],required=True);p.add_argument('--commit',required=True);a=p.parse_args()
r=pathlib.Path(__file__).resolve().parents[1]
fixed={'fheroes2KM.exe','files/data/resurrectionKM.h2d','files/data/training_grounds.png','files/data/build_training_grounds.png','knight_mod.ini.default','Uninstall_KM.bat','LICENCE','KM_SOURCE_COMMIT.txt','KM_BUILD.txt'}
langs=set('be bg cs da de el eo es fr hu it lt nb nl pl pt ro ru sk sv tr uk vi'.split())
expected=fixed|{f'files/lang/km_{x}.mo' for x in langs}
with zipfile.ZipFile(a.archive) as z:
    names=[n.replace('\\','/') for n in z.namelist() if not n.endswith('/')]
    assert len(names)==len(set(names)) and set(names)==expected,(set(names)-expected,expected-set(names))
    for n in names:assert not pathlib.PurePosixPath(n).is_absolute() and '..' not in pathlib.PurePosixPath(n).parts
    assert z.testzip() is None
    def read(n):return z.read(next(x for x in z.namelist() if x.replace('\\','/')==n))
    assert read('KM_SOURCE_COMMIT.txt').decode('ascii').strip()==a.commit
    assert f'Architecture: {a.arch}' in read('KM_BUILD.txt').decode('ascii')
    b=read('fheroes2KM.exe');assert b[:2]==b'MZ';off=struct.unpack_from('<I',b,0x3c)[0];assert b[off:off+4]==b'PE\0\0'
    machine=struct.unpack_from('<H',b,off+4)[0];assert machine=={'x86':0x14c,'x64':0x8664}[a.arch]
    assert struct.unpack_from('<H',b,off+24+68)[0]==2 # GUI subsystem
    for k in [b'[KNIGHT ECONOMY]',b'[TAXPAYER DIAG]',b'[TAXPAYER AI ECONOMY]',b'NAVAL_OPENING_J1',b'OBSERVER_KINGDOM',b'POPULATION_SUPPLY_PLAN']:assert k not in b,k
    for k in [b'Knight Mod Public Beta',b'taxpayer.max_taxable_peasants_per_farm']:assert k in b,k
    for n in langs:
        payload=read(f'files/lang/km_{n}.mo');assert payload==(r/f'files/lang/km_{n}.mo').read_bytes()
        catalog=gettext.GNUTranslations(io.BytesIO(payload));assert catalog.gettext('Training Grounds')!='Training Grounds'
    for pkg,src in [('files/data/resurrectionKM.h2d','files/data/resurrection.h2d'),('files/data/training_grounds.png','files/data/training_grounds.png'),('files/data/build_training_grounds.png','files/data/build_training_grounds.png')]:assert read(pkg)==(r/src).read_bytes(),pkg
    for pkg,src in [('knight_mod.ini.default','packaging/km/knight_mod.ini.default'),('Uninstall_KM.bat','packaging/km/Uninstall_KM.bat'),('LICENCE','LICENSE')]:assert read(pkg).replace(b'\r\n',b'\n')==(r/src).read_bytes().replace(b'\r\n',b'\n'),pkg
print(json.dumps(dict(file=a.archive.name,architecture=a.arch,source=a.commit,files=len(expected),languages=len(langs),sha256=hashlib.sha256(a.archive.read_bytes()).hexdigest(),result='PASS'),indent=2))
