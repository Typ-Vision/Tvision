"""Check the small handoff bundle and its documentation links (Python stdlib only).

Checks inline Markdown links outside fenced code, local Markdown fragments,
UTF-8, startup byte limits, and immutable migration archive hashes. It does not
check web availability, product behavior, or the contents of build evidence.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
from urllib.parse import unquote, urlsplit


LIMITS = {"AGENTS.md": 4000, "README.md": 8000, "docs/progress.md": 8000}
ACTIVE = [
    "docs/architecture/industrial-camera-plan.md",
    "docs/architecture/industrial-camera-handoff.md",
    "docs/architecture/industrial-camera-tasks-foundation.md",
    "docs/architecture/industrial-camera-tasks-integration.md",
]
LINK = re.compile(r'\]\((?:<([^>]+)>|([^\s)]+))(?:\s+"[^"]*")?\)')


def prose(text):
    """Remove fenced examples; their sample URLs are not live document links."""
    lines = []
    fence = None
    for line in text.splitlines():
        match = re.match(r"^\s{0,3}(`{3,}|~{3,})", line)
        if fence:
            if match and match[1][0] == fence[0] and len(match[1]) >= len(fence):
                fence = None
        elif match:
            fence = match[1]
        else:
            lines.append(line)
    return "\n".join(lines)


def anchors(text):
    content = prose(text)
    result = set(re.findall(r'<a\s+(?:id|name)=["\']([^"\']+)["\']', content))
    seen = {}
    for title in re.findall(r"^#{1,6}\s+(.+?)(?:\s+#+)?$", content, re.MULTILINE):
        title = re.sub(r"\[([^]]+)\]\([^)]*\)", r"\1", title)
        slug = re.sub(r"[^\w\-\s]", "", title.lower()).replace(" ", "-")
        count = seen.get(slug, 0)
        seen[slug] = count + 1
        result.add(slug if count == 0 else f"{slug}-{count}")
    return result


def check(root):
    errors, sizes, cache = [], {}, {}
    links = 0

    def read(path):
        if path not in cache:
            cache[path] = path.read_text(encoding="utf-8-sig", errors="strict")
        return cache[path]

    for relative, limit in LIMITS.items():
        path = root / relative
        if not path.is_file():
            errors.append(f"Missing startup file: {relative}")
            continue
        sizes[relative] = path.stat().st_size
        if sizes[relative] > limit:
            errors.append(f"Byte limit exceeded: {relative}: {sizes[relative]} > {limit}")
    total = sum(sizes.values())
    if total > 20000:
        errors.append(f"Startup total exceeds 20000 bytes: {total}")

    files = {root / p for p in [*LIMITS, *ACTIVE]}
    files.update((root / "docs/requirements").glob("*.md"))
    files.update((root / "docs/architecture/camera-setup").glob("*.md"))
    files.add(root / "docs/camera-runtime-usage.md")
    files.update((root / "docs/archive").rglob("*.md"))
    for source in sorted(files):
        relative = source.relative_to(root).as_posix()
        try:
            content = prose(read(source))
        except (OSError, UnicodeError) as exc:
            errors.append(f"Cannot read {relative}: {exc}")
            continue
        for match in LINK.finditer(content):
            url = match[1] or match[2]
            parsed = urlsplit(url)
            if parsed.scheme or parsed.netloc:
                continue
            links += 1
            target = (source.parent / unquote(parsed.path)).resolve() if parsed.path else source
            if not target.exists():
                errors.append(f"Missing target in {relative}: {url}")
            elif parsed.fragment and target.suffix.lower() == ".md":
                try:
                    if unquote(parsed.fragment) not in anchors(read(target)):
                        errors.append(f"Missing fragment in {relative}: {url}")
                except (OSError, UnicodeError) as exc:
                    errors.append(f"Cannot read link target in {relative}: {exc}")

    archived = 0
    manifest_path = root / "docs/archive/2026-10-17/manifest.json"
    try:
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        for item in manifest["archives"]:
            path = (root / item["path"]).resolve()
            if not path.is_relative_to(root / "docs/archive"):
                errors.append(f"Archive path escapes archive directory: {item['path']}")
                continue
            # Git autocrlf may change line endings on Windows; content integrity
            # must survive checkout without ignoring any other byte difference.
            if not path.is_file() or hashlib.sha256(path.read_bytes().replace(b"\r\n", b"\n")).hexdigest() != item["sha256_lf"]:
                errors.append(f"Archive hash mismatch: {item['path']}")
            archived += 1
    except (OSError, UnicodeError, ValueError, KeyError, TypeError) as exc:
        errors.append(f"Invalid archive manifest: {exc}")

    return {"ok": not errors, "startup_bytes": sizes, "startup_total_bytes": total,
            "checked_documents": len(files), "local_links": links,
            "archive_hashes": archived, "errors": errors}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1],
                        help="Repository root (also supports isolated validation fixtures)")
    args = parser.parse_args()
    report = check(args.root.resolve())
    print(json.dumps(report, indent=2, ensure_ascii=True))
    raise SystemExit(0 if report["ok"] else 1)
