"""Scores GGTests --classify output against the labels.
   python3 -I score.py <labels.json> <classify.tsv> [-w]     (-w lists the wrong ones)"""
import json, csv, sys
from collections import Counter, defaultdict
cat = {'Kick': 'kick', '808': '808', 'Snare': 'snare', 'Clap': 'clap', 'Rim': 'rim', 'Closed Hat': 'closedhat', 'Open Hat': 'openhat',
       'Tom': 'tom', 'Crash': 'crash', 'Ride': 'ride', 'Perc': 'perc', 'FX': 'fx', 'Loop': 'loop', 'Vocal / Melodic': 'melodic', 'Other': 'unknown'}
labels = json.load(open(sys.argv[1]))
ok = tot = 0
conf = defaultdict(Counter)
wrong = []
for r in csv.DictReader(open(sys.argv[2]), delimiter='\t'):
    if r['path'] not in labels: continue
    lab, got = labels[r['path']], cat.get(r['cat'], r['cat'])
    good = set(lab.split('|')) | ({'crash', 'ride'} if ('crash' in lab or 'ride' in lab) else set())
    tot += 1; ok += got in good
    conf[lab][got] += 1
    if got not in good: wrong.append((lab, got, '/'.join(r['path'].split('/')[-2:])))
print(f'{ok}/{tot} = {100 * ok / max(1, tot):.1f}%')
for lab in sorted(conf): print(f'  {lab:18s}', dict(conf[lab].most_common()))
if '-w' in sys.argv:
    for w in wrong: print('  ', *w)
