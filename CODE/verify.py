"""Verify the published payload and statically audited localization coverage."""
import argparse
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--game-dir", type=Path)
args = parser.parse_args()
summary = json.loads((root / "audit/validation_summary.json").read_text(encoding="utf-8"))
checksums = json.loads((root / "checksums.json").read_text(encoding="utf-8"))
for name, expected in checksums.items():
    actual = hashlib.sha256((root / name).read_bytes()).hexdigest()
    if actual != expected:
        raise SystemExit(f"Payload checksum mismatch: {name}")

translations = json.loads((root / "loc/eng_us/localize.json").read_text(encoding="utf-8"))["translations"]
fields = json.loads((root / "audit/table_field_keys.json").read_text(encoding="utf-8"))
required = [prefix + field for field in fields for prefix in ("table_header_", "table_header_tooltip_")]
editor_fields = json.loads((root / "audit/editor_field_keys.json").read_text(encoding="utf-8"))
required += [field + suffix for field in editor_fields for suffix in ("_abbr", "_tooltip")]
required += ["contract", "attr_matrix_attack", "finalize_loan", "maverick", "heartbeat", "virtuoso"]
required += [f"coaches_tab_{n}" for n in range(4)]
missing = [key for key in required if not translations.get(key)]
if missing:
    raise SystemExit(f"Missing translation keys: {missing}")
if len(translations) != summary["translations"]:
    raise SystemExit("Translation count differs from the published audit")
if args.game_dir:
    dll = args.game_dir / "FCLiveEditor.DLL"
    if hashlib.sha256(dll.read_bytes()).hexdigest() != summary["target_sha256"]:
        raise SystemExit("Unsupported FCLiveEditor.DLL: version hash differs")
print(f"Verified {len(translations)} translations, {len(fields)} table fields, {len(editor_fields)} editor fields and {len(checksums)} payload files.")
