"""Storage layer: writes data to disk and reads it back.

This layer is deliberately "dumb": it only handles raw dicts and does not know about the Task
class. That keeps the responsibilities separate — manager.py "translates", storage.py "moves".
"""

from __future__ import annotations

import json
from pathlib import Path


class JsonStorage:
    """A storage implementation that keeps data in a JSON file."""

    def __init__(self, path: str | Path) -> None:
        # Path handles strings/path objects uniformly, more reliably than gluing strings.
        self.path = Path(path)

    def load(self) -> list[dict]:
        """Read the file and return a list of dicts.

        A missing file, empty content or damaged format all mean "no data yet":
        return an empty list instead of letting the program crash.
        """
        if not self.path.exists():  # file does not exist
            return []

        text = self.path.read_text(encoding="utf-8").strip()
        if not text:
            return []

        try:
            data = json.loads(text)
        except json.JSONDecodeError:
            print(f"⚠️  Data file is malformed, treating it as empty for now: {self.path}")
            # Rename the bad file as a backup and continue with empty data,
            # so the user's data does not silently disappear.
            backup = self.path.with_suffix(self.path.suffix + ".bak")
            self.path.replace(backup)
            print(f"📦 Backed up to: {backup}")
            return []

        if not isinstance(data, list):
            return []
        return [item for item in data if isinstance(item, dict)]

    def save(self, data: list[dict]) -> None:
        """Write the list of dicts back to the file.

        parents=True        create missing parent directories along the way;
        exist_ok=True       no error if the directory already exists;
        ensure_ascii=False  write non-ASCII text as-is so it stays readable in the file;
        indent=2            pretty-print, handy when you open the file to debug.
        """
        self.path.parent.mkdir(parents=True, exist_ok=True)
        self.path.write_text(
            json.dumps(data, ensure_ascii=False, indent=2),
            encoding="utf-8",
        )
