"""Builds the labelled sets from the downloads (run fetch.sh first):
   $AUDIT/{dirt,tdm}/named     the files with their own names and folders (what a real library looks like)
   $AUDIT/{dirt,tdm}/scrubbed  the same audio as "Pack/Sound 0001.wav" (the sound alone, like Harvest)
   and labels_named.json / labels_scrubbed.json next to them. 'a|b' = either answer is right."""
import os, re, json
AUDIT = os.environ.get('AUDIT', '/tmp/gg-audit')

def write_sets(root, labels):
    named, scrub = {}, {}
    for n, (src, rel, lab) in enumerate(sorted(labels)):
        a = os.path.join(root, 'named', rel)
        b = os.path.join(root, 'scrubbed', 'Pack', 'Sound %04d.wav' % (n + 1))
        for d in (a, b):
            os.makedirs(os.path.dirname(d), exist_ok=True)
            if not os.path.exists(d): os.link(src, d)
        named[a] = lab; scrub[b] = lab
    json.dump(named, open(os.path.join(root, 'labels_named.json'), 'w'))
    json.dump(scrub, open(os.path.join(root, 'labels_scrubbed.json'), 'w'))
    print(root, len(named))

# ---- Dirt-Samples: one folder per kind, a few mixed kits labelled by file name ----
D = os.path.join(AUDIT, 'dirt')
dl = []
def folder(name, lab, pattern=None):
    p = os.path.join(D, name)
    if not os.path.isdir(p): return
    for f in sorted(os.listdir(p)):
        if f.lower().endswith(('.wav', '.aif', '.aiff')) and (not pattern or re.search(pattern, f, re.I)):
            dl.append((os.path.join(p, f), name + '/' + f, lab))
for n in ['bd', 'clubkick', 'hardkick', 'kicklinn', 'popkick', 'reverbkick', 'bassdm']: folder(n, 'kick')
for f in sorted(os.listdir(os.path.join(D, '808bd'))):
    m = re.match(r'BD(\d\d)(\d\d)', f, re.I)       # BD<tone><decay>: long decays are 808 booms
    dl.append((os.path.join(D, '808bd', f), '808bd/' + f, 'kick' if (int(m.group(2)) if m else 0) <= 25 else 'kick|808'))
for n in ['sd', 'sn', '808sd']: folder(n, 'snare')
for n in ['cp', 'realclaps']: folder(n, 'clap')
for n in ['hc', 'linnhats']: folder(n, 'closedhat')
for n in ['ho', '808oh']: folder(n, 'openhat')
for n in ['808lt', '808mt', '808ht', 'lt', 'mt', 'ht']: folder(n, 'tom')
for n in ['808cy', 'cr']: folder(n, 'crash|ride')
for n in ['rm', 'rs']: folder(n, 'rim')
for n in ['perc', 'cb', '808lc', '808mc', '808hc', 'tabla']: folder(n, 'perc')
for n in ['hit', 'space', 'wind', 'noise2', 'noise']: folder(n, 'fx')
folder('jungbass', '808', r'808|sub|low|deep|boom')
folder('jungbass', 'bass|808|melodic', r'^(?!.*(808|sub|low|deep|boom))')
folder('bass1', 'bass|808|melodic')
folder('stab', 'melodic|fx')                        # the app files stabs as FX (orchestra hits)
for f in os.listdir(os.path.join(D, '808')):
    k = os.path.splitext(f)[0].upper()
    lab = {'CB': 'perc', 'CH': 'closedhat', 'CL': 'perc', 'CP': 'clap', 'HC': 'perc', 'MA': 'perc', 'OH': 'openhat', 'RS': 'rim'}.get(k)
    if lab: dl.append((os.path.join(D, '808', f), '808/' + f, lab))
rules = [(r'closed|chh|_ch\b|hhc|closedhh', 'closedhat'), (r'open|ohh|hho', 'openhat'), (r'crash|ride', 'crash|ride'), (r'snare', 'snare'),
         (r'clap', 'clap'), (r'tom', 'tom'), (r'kick|_bd|bd\.|bass ?drum', 'kick'), (r'cowbell|cabasa|tamb|conga|bongo|clave|shaker|_cb', 'perc'),
         (r'rim|_rs', 'rim'), (r'_fx|fx\.', 'fx'), (r'_sd|sd\.', 'snare'), (r'_hh|hh\.', 'closedhat')]
for n in ['gretsch', 'drumtraks', 'electro1', 'house', 'glitch', 'hh']:
    p = os.path.join(D, n)
    for f in os.listdir(p) if os.path.isdir(p) else []:
        if f.lower().endswith('.wav'):
            for pat, lab in rules:
                if re.search(pat, f, re.I): dl.append((os.path.join(p, f), n + '/' + f, lab)); break
write_sets(os.path.join(AUDIT, 'dirt-set'), dl)

# ---- tidal-drum-machines: the folder suffix says what it is (misc left out) ----
kinds = {'bd': 'kick', 'sd': 'snare', 'hh': 'closedhat', 'oh': 'openhat', 'cp': 'clap', 'cr': 'crash|ride', 'rd': 'crash|ride',
         'lt': 'tom', 'mt': 'tom', 'ht': 'tom', 'rim': 'rim', 'sh': 'perc', 'cb': 'perc', 'tb': 'perc', 'perc': 'perc', 'fx': 'fx'}
tl = []
for root, dirs, files in sorted(os.walk(os.path.join(AUDIT, 'tdm', 'machines'))):
    m = re.match(r'[a-z0-9]+-([a-z]+)\d*$', os.path.basename(root))
    if not m or m.group(1) not in kinds: continue
    machine = os.path.basename(os.path.dirname(root))
    lab = 'kick|808' if kinds[m.group(1)] == 'kick' and machine == 'RolandTR808' else kinds[m.group(1)]
    for f in sorted(files):
        if f.lower().endswith('.wav'):
            tl.append((os.path.join(root, f), machine + '/' + os.path.basename(root) + '/' + f, lab))
write_sets(os.path.join(AUDIT, 'tdm-set'), tl)
