from subprocess import TimeoutExpired, run
from pathlib import Path

for submission_path in (Path() / "submissions").iterdir():
    try:
        p = run(
            [
                (Path() / ".." / "code" / "ex7" / "check_zip.sh").resolve(),
                submission_path.name,
            ],
            capture_output=True,
            text=True,
            cwd=Path() / "submissions",
            timeout=3,
        )
    except TimeoutExpired:
        print(submission_path)
        print("TIMEOUT")
    else:
        if p.returncode != 0:
            print(submission_path)
            print(p.stdout)
