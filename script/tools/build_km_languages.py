"""Build isolated km_ catalogs from upstream PO files and Knight overlays (stdlib only)."""
import ast, gettext, json, pathlib, re, struct, sys, unicodedata
ROOT=pathlib.Path(__file__).resolve().parents[2]
KEYS=["Training Grounds", "Convert the Farm into Training Grounds to enable Peasant conscription. The Farm's growth bonus will be lost.", " Peasants stationed here will no longer pay taxes.", "Restore the Farm and disable conscription.\n\n", "Automatically trains up to %{count} Peasants per day into infantry.", "Conscripts are taken from the garrison first: %{fee} gold each. Peasants taken from the Thatched Hut also cost %{price} gold to recruit.", "Trained troops enter this town's recruitment pools and must be purchased normally.", "Taxable Peasants per Farm: %{count}."]
EXTRA=["Conscription is disabled in knight_mod.ini.","Training success: %{percent}%. Failed conscripts are lost and still paid for.","Expected Tomorrow:","No troops"]
def load_po(path):
    catalog={}; entry={}; field=None; fuzzy=False
    def flush():
        if 'msgid' in entry and not fuzzy:
            key=entry['msgid']; value=entry.get('msgstr','')
            if 'msgid_plural' in entry:
                key+='\0'+entry['msgid_plural']; value='\0'.join(entry.get('msgstr['+str(i)+']','') for i in range(len([k for k in entry if k.startswith('msgstr[')])))
            if 'msgctxt' in entry:key=entry['msgctxt']+'\x04'+key
            if value:catalog[key]=value
    for line in path.read_text(encoding='utf-8-sig').splitlines()+['']:
        if not line.strip():flush();entry={};field=None;fuzzy=False;continue
        if line.startswith('#,') and 'fuzzy' in line:fuzzy=True
        if line.startswith('#'):continue
        m=re.match(r'(msgctxt|msgid_plural|msgid|msgstr(?:\[\d+\])?)\s+(".*")$',line)
        if m:field=m[1];entry[field]=ast.literal_eval(m[2])
        elif line.startswith('"') and field:entry[field]+=ast.literal_eval(line)
    return catalog
CP={**dict.fromkeys(['cs','hu','pl','sk'],'cp1250'),**dict.fromkeys(['be','bg','ru','uk'],'cp1251'),**dict.fromkeys(['de','da','fr','it','nl','nb','pt','es','sv'],'cp1252'),'el':'cp1253','tr':'cp1254','lt':'cp1257','vi':'cp1258','ro':'iso8859_16','eo':'iso8859_3'}
def rows(name):
    return {line.split('|')[0]:line.split('|')[1:] for line in (ROOT/'packaging/km/translations'/name).read_text().splitlines() if line and not line.startswith('#')}
def encode(s,cp):
    try:return s.encode(cp)
    except UnicodeEncodeError:
        # Vietnamese is represented with combining marks in CP1258.
        if cp=='cp1258':
            tones={'\u0300','\u0301','\u0303','\u0309','\u0323'}; groups=[]
            for ch in unicodedata.normalize('NFD',s):
                if not unicodedata.combining(ch):groups.append(ch)
                elif groups:groups[-1]+=ch
            rebuilt=''
            for group in groups:
                rebuilt+=unicodedata.normalize('NFC',''.join(c for c in group if c not in tones))+''.join(c for c in group if c in tones)
            return rebuilt.encode(cp,errors='replace')
        return s.encode(cp,errors='replace')
def mo(catalog,cp):
    pairs=sorted((encode(k,cp),encode(v,cp)) for k,v in catalog.items());n=len(pairs);start=28+16*n;ids=b'';values=b'';it=[];vt=[]
    for k,v in pairs:it.append((len(k),start+len(ids)));ids+=k+b'\0'
    for k,v in pairs:vt.append((len(v),start+len(ids)+len(values)));values+=v+b'\0'
    return struct.pack('<7I',0x950412de,0,n,28,28+8*n,0,0)+b''.join(struct.pack('<2I',*x) for x in it+vt)+ids+values
base=rows('core.tsv');extra=rows('extra.tsv');out=pathlib.Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'files/lang';out.mkdir(parents=True,exist_ok=True)
for lang,translations in base.items():
    assert len(translations)==len(KEYS),(lang,len(translations))
    translations[3]+='\n\n'
    updates=dict(zip(KEYS+EXTRA,translations+extra[lang]));catalog=load_po(ROOT/'files/lang'/f'{lang}.po');catalog.update(updates);cp=CP[lang]
    for k,v in updates.items():assert unicodedata.normalize('NFC',encode(v,cp).decode(cp))==unicodedata.normalize('NFC',v),(lang,'unrepresentable translation',v)
    for k,v in updates.items():assert sorted(re.findall(r'%\{[^}]+\}',k))==sorted(re.findall(r'%\{[^}]+\}',v)),(lang,k)
    header=catalog.get('','');header=re.sub(r'charset=[^\s\\]+','charset='+cp,header);
    if 'charset=' not in header:header+='\nContent-Type: text/plain; charset='+cp+'\n'
    catalog['']=header.encode('ascii',errors='replace').decode('ascii')
    path=out/f'km_{lang}.mo';path.write_bytes(mo(catalog,cp))
    with path.open('rb') as f:g=gettext.GNUTranslations(f)
    for k,v in updates.items():assert g.gettext(k)==encode(v,cp).decode(cp),(lang,k)
    print(lang,len(catalog),path.name)
