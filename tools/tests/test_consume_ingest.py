from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest


MODULE_PATH = Path(__file__).resolve().parents[1] / "consume_ingest.py"
SPEC = importlib.util.spec_from_file_location("consume_ingest", MODULE_PATH)
consume_ingest = importlib.util.module_from_spec(SPEC)
assert SPEC and SPEC.loader
SPEC.loader.exec_module(consume_ingest)


class ConsumeIngestTests(unittest.TestCase):
    def test_valid_record_is_sha_and_length_verified(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            obj = root / "objects" / "sha256" / "aa" / "sample.bin"
            obj.parent.mkdir(parents=True)
            data = b"raw\x00bytes\xff"
            obj.write_bytes(data)
            record = {
                "schema": consume_ingest.SCHEMA,
                "source_id": "raw-web:sha256:test",
                "object_path": obj.relative_to(root).as_posix(),
                "byte_length": len(data),
                "sha256": hashlib.sha256(data).hexdigest(),
            }
            source_id, path = consume_ingest.validate_record(record, root)
            self.assertEqual(source_id, record["source_id"])
            self.assertEqual(path, obj.resolve())

    def test_capture_id_is_preferred_as_structural_source_identity(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            obj = root / "sample.bin"
            data = b"same-content"
            obj.write_bytes(data)
            record = {
                "schema": consume_ingest.SCHEMA,
                "source_id": "raw-web:sha256:" + hashlib.sha256(data).hexdigest(),
                "capture_id": "web:capture-123",
                "object_path": "sample.bin",
                "byte_length": len(data),
                "sha256": hashlib.sha256(data).hexdigest(),
            }
            source_id, _ = consume_ingest.validate_record(record, root)
            self.assertEqual(source_id, "web:capture-123")

    def test_path_traversal_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "root"
            root.mkdir()
            outside = Path(tmp) / "outside.bin"
            outside.write_bytes(b"x")
            record = {
                "schema": consume_ingest.SCHEMA,
                "source_id": "x",
                "object_path": "../outside.bin",
                "byte_length": 1,
                "sha256": hashlib.sha256(b"x").hexdigest(),
            }
            with self.assertRaisesRegex(ValueError, "escapes"):
                consume_ingest.validate_record(record, root)

    def test_pending_reader_returns_commit_offset_only_through_selected_records(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            objects = root / "objects"
            objects.mkdir()
            spool = root / "bit-analyze-ingest.jsonl"
            rows = []
            for i in range(3):
                data = f"record-{i}".encode()
                obj = objects / f"{i}.bin"
                obj.write_bytes(data)
                rows.append({
                    "schema": consume_ingest.SCHEMA,
                    "source_id": f"source-{i}",
                    "object_path": obj.relative_to(root).as_posix(),
                    "byte_length": len(data),
                    "sha256": hashlib.sha256(data).hexdigest(),
                })
            encoded = [json.dumps(row).encode() + b"\n" for row in rows]
            spool.write_bytes(b"".join(encoded))

            selected, offset = consume_ingest.load_pending(
                spool, root, cursor_offset=0, max_records=2
            )
            self.assertEqual([x[0] for x in selected], ["source-0", "source-1"])
            self.assertEqual(offset, len(encoded[0]) + len(encoded[1]))

            selected2, offset2 = consume_ingest.load_pending(
                spool, root, cursor_offset=offset, max_records=2
            )
            self.assertEqual([x[0] for x in selected2], ["source-2"])
            self.assertEqual(offset2, spool.stat().st_size)

    def test_sha_mismatch_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            obj = root / "sample.bin"
            obj.write_bytes(b"actual")
            record = {
                "schema": consume_ingest.SCHEMA,
                "source_id": "x",
                "object_path": "sample.bin",
                "byte_length": len(b"actual"),
                "sha256": hashlib.sha256(b"different").hexdigest(),
            }
            with self.assertRaisesRegex(ValueError, "SHA-256 mismatch"):
                consume_ingest.validate_record(record, root)


    def test_checkpoint_pointer_commits_cursor_and_state_together(self):
        with tempfile.TemporaryDirectory() as tmp:
            checkpoint_dir = Path(tmp)
            state = checkpoint_dir / "state-10.bin"
            events = checkpoint_dir / "events-10.jsonl"
            state.write_bytes(b"state")
            events.write_text("{}\n", encoding="utf-8")
            payload = {
                "schema": consume_ingest.CHECKPOINT_SCHEMA,
                "hierarchy_id": "hierarchy:test",
                "lineage_origin": "created",
                "structural_config": {"window": 4096, "hop": 4096, "layers": 2},
                "cursor_offset": 10,
                "previous_cursor_offset": 0,
                "state_file": state.name,
                "events_file": events.name,
                "checkpoint_file": "checkpoint-10.json",
                "record_count": 1,
                "spool": "/tmp/spool.jsonl",
                "previous_state_file": None,
                "previous_checkpoint_file": None,
            }
            consume_ingest.commit_checkpoint(checkpoint_dir, payload)
            offset, state_path, loaded = consume_ingest.load_checkpoint(checkpoint_dir)
            self.assertEqual(offset, 10)
            self.assertEqual(state_path, state.resolve())
            self.assertEqual(loaded["events_file"], events.name)
            self.assertEqual(loaded["hierarchy_id"], "hierarchy:test")
            self.assertTrue((checkpoint_dir / "checkpoint-10.json").is_file())

    def test_lineage_reuses_id_and_rejects_structural_config_change(self):
        config = {"window": 4096, "hop": 4096, "layers": 2}
        first_id, origin = consume_ingest.resolve_lineage(None, config)
        self.assertTrue(first_id.startswith("hierarchy:"))
        self.assertEqual(origin, "created")

        previous = {
            "hierarchy_id": first_id,
            "lineage_origin": origin,
            "structural_config": dict(config),
        }
        second_id, second_origin = consume_ingest.resolve_lineage(previous, dict(config))
        self.assertEqual(second_id, first_id)
        self.assertEqual(second_origin, origin)

        with self.assertRaisesRegex(ValueError, "configuration changed"):
            consume_ingest.resolve_lineage(
                previous,
                {"window": 8192, "hop": 4096, "layers": 2},
            )

    def test_legacy_checkpoint_can_be_adopted_into_new_lineage(self):
        hierarchy_id, origin = consume_ingest.resolve_lineage(
            {"schema": consume_ingest.LEGACY_CHECKPOINT_SCHEMA},
            {"window": 4096, "hop": 4096, "layers": 2},
        )
        self.assertTrue(hierarchy_id.startswith("hierarchy:"))
        self.assertEqual(origin, "legacy_checkpoint_adopted")

    def test_checkpoint_path_escape_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            checkpoint_dir = Path(tmp) / "checkpoint"
            checkpoint_dir.mkdir()
            outside = Path(tmp) / "outside.bin"
            outside.write_bytes(b"x")
            (checkpoint_dir / "current.json").write_text(
                json.dumps({
                    "schema": consume_ingest.CHECKPOINT_SCHEMA,
                    "cursor_offset": 1,
                    "state_file": "../outside.bin",
                    "events_file": "",
                }),
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "escapes"):
                consume_ingest.load_checkpoint(checkpoint_dir)


if __name__ == "__main__":
    unittest.main()
