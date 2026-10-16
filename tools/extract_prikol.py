"""Extract composited GIF frames and their original timings for Unreal import."""
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parents[1]
out = root / "tmp" / "prikol"
out.mkdir(parents=True, exist_ok=True)
durations = []
with Image.open(root / "dz1" / "prikoli" / "prikol.gif") as gif:
    for index in range(gif.n_frames):
        gif.seek(index)
        gif.convert("RGBA").save(out / f"Frame_{index:03d}.png")
        durations.append(str(max(gif.info.get("duration", 100), 10) / 1000.0))
(out / "durations.txt").write_text("\n".join(durations), encoding="utf-8")
print(f"Extracted {len(durations)} frames, {sum(map(float, durations)):.3f} seconds")
