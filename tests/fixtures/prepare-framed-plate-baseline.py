"""Freeze the external CCUX-15f raw+sidecar baseline after checking the tracked manifest.
Run from the repository root. Requires Pillow only for the independent oracle preview.
Does not capture hardware, regenerate expected centres, or turn a missing raw into success.
"""
from pathlib import Path
import hashlib
import json
import shutil
from PIL import Image, ImageDraw, ImageEnhance

repo = Path(__file__).resolve().parents[2]
manifest = json.loads((repo / 'tests/fixtures/framed-plate-external.json').read_text())
for fixture in manifest['fixtures']:
    source, dest = repo / fixture['sourcePath'], repo / fixture['baselinePath']
    dest.parent.mkdir(parents=True, exist_ok=True)
    for suffix, key in [('', 'fileSha256'), ('.json', 'sidecarSha256')]:
        original, snapshot = Path(str(source) + suffix), Path(str(dest) + suffix)
        digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
        if digest(original) != fixture[key]:
            raise ValueError(f'Original bytes changed: {original}')
        if snapshot.exists() and digest(snapshot) != fixture[key]:
            raise ValueError(f'Immutable baseline changed: {snapshot}')
        if not snapshot.exists():
            shutil.copyfile(original, snapshot)
    oracle = json.loads((repo / fixture['oraclePath']).read_text())
    image = Image.open(source).convert('RGB')
    image = ImageEnhance.Brightness(image).enhance(3 if fixture['id'] == 'plate-1' else 1.6)
    draw = ImageDraw.Draw(image)
    for number, (x, y) in enumerate(oracle['centresRaw'], 1):
        draw.ellipse((x-8, y-8, x+8, y+8), outline='lime', width=2)
        draw.text((x+10, y-12), str(number), fill='yellow')
    image.save(dest.with_name(dest.stem + '-oracle.png'))
legacy = manifest['legacyWrong49']
original, snapshot = repo / legacy['sourcePath'], repo / legacy['baselinePath']
for path in [original] + ([snapshot] if snapshot.exists() else []):
    if hashlib.sha256(path.read_bytes()).hexdigest() != legacy['fileSha256']:
        raise ValueError(f'Historical rejected diagnostic changed: {path}')
if not snapshot.exists():
    shutil.copyfile(original, snapshot)
print('Two immutable raw/sidecar baselines and rejected historical diagnostic verified; independent oracle previews saved.')
