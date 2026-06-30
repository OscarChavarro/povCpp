#!/usr/bin/env python3
"""
Benchmark renderAll.sh across repository commits and write timing data to CSV.

The script uses a detached git worktree under /tmp so the current checkout stays
untouched. Every commit is rebuilt after checkout, but the CSV only stores the
wall-clock time of scripts/renderAll.sh.
"""

from __future__ import annotations

import argparse
import csv
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Dict, Iterable, List, Sequence


def parse_args() -> argparse.Namespace:
    repo_root = Path(__file__).resolve().parent.parent

    parser = argparse.ArgumentParser(
        description=(
            "Recorre commits desde HEAD hacia atras, ejecuta scripts/renderAll.sh "
            "y escribe estadisticas CSV."
        )
    )
    parser.add_argument(
        "--repo",
        type=Path,
        default=repo_root,
        help="Raiz del repositorio a medir.",
    )
    parser.add_argument(
        "--output-csv",
        type=Path,
        default=Path("/tmp/stats.csv"),
        help="CSV de salida.",
    )
    parser.add_argument(
        "--worktree-dir",
        type=Path,
        default=Path("/tmp/povCpp-perf-worktree"),
        help="Worktree temporal usado para recorrer commits.",
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=None,
        help="Cantidad maxima de commits a procesar desde HEAD.",
    )
    parser.add_argument(
        "--compile-command",
        nargs="+",
        default=["scripts/compile.sh"],
        help="Comando de compilacion relativo al worktree. Se ejecuta en cada commit.",
    )
    parser.add_argument(
        "--render-command",
        nargs="+",
        default=["scripts/renderAll.sh"],
        help="Comando de render relativo al worktree.",
    )
    parser.add_argument(
        "--skip-existing",
        action="store_true",
        help="Si el CSV ya existe, salta commits que ya tengan fila.",
    )
    parser.add_argument(
        "--keep-worktree",
        action="store_true",
        help="No elimina el worktree temporal al terminar.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Lista los commits objetivo y termina sin ejecutar nada.",
    )
    parser.add_argument(
        "--env",
        action="append",
        default=[],
        metavar="NAME=VALUE",
        help="Variables de entorno extra para compile/render. Puede repetirse.",
    )
    return parser.parse_args()


def run_command(
    command: Sequence[str],
    *,
    cwd: Path,
    env: Dict[str, str] | None = None,
    capture_output: bool = True,
) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        list(command),
        cwd=str(cwd),
        env=env,
        text=True,
        capture_output=capture_output,
        check=False,
    )


def git(repo: Path, *args: str) -> subprocess.CompletedProcess[str]:
    return run_command(["git", "-C", str(repo), *args], cwd=repo)


def list_commits(repo: Path, limit: int | None) -> List[str]:
    result = git(repo, "rev-list", "HEAD")
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "git rev-list HEAD failed")
    commits = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    if limit is not None:
        commits = commits[:limit]
    return commits


def read_existing_commits(csv_path: Path) -> set[str]:
    if not csv_path.exists():
        return set()

    with csv_path.open(newline="", encoding="utf-8") as handle:
        reader = csv.DictReader(handle)
        return {
            row["commit"]
            for row in reader
            if row.get("commit")
        }


def parse_env_pairs(items: Iterable[str]) -> Dict[str, str]:
    parsed: Dict[str, str] = {}
    for item in items:
        if "=" not in item:
            raise ValueError(f"Invalid --env value: {item!r}")
        key, value = item.split("=", 1)
        if not key:
            raise ValueError(f"Invalid --env key in: {item!r}")
        parsed[key] = value
    return parsed


def commit_metadata(repo: Path, commit: str) -> Dict[str, str]:
    result = git(
        repo,
        "show",
        "-s",
        "--format=%H%n%h%n%ci%n%s",
        commit,
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or f"git show failed for {commit}")
    full_hash, short_hash, commit_date, subject = result.stdout.splitlines()[:4]
    return {
        "commit": full_hash,
        "short_commit": short_hash,
        "commit_date": commit_date,
        "subject": subject,
    }


def ensure_clean_path(path: Path) -> None:
    if path.is_symlink() or path.is_file():
        path.unlink()
    elif path.is_dir():
        shutil.rmtree(path)


def setup_worktree(repo: Path, worktree_dir: Path, first_commit: str) -> None:
    remove_result = git(repo, "worktree", "remove", "--force", str(worktree_dir))
    if remove_result.returncode != 0 and worktree_dir.exists():
        shutil.rmtree(worktree_dir, ignore_errors=True)

    worktree_dir.parent.mkdir(parents=True, exist_ok=True)

    add_result = git(repo, "worktree", "add", "--detach", str(worktree_dir), first_commit)
    if add_result.returncode != 0:
        raise RuntimeError(add_result.stderr.strip() or "git worktree add failed")


def teardown_worktree(repo: Path, worktree_dir: Path) -> None:
    remove_result = git(repo, "worktree", "remove", "--force", str(worktree_dir))
    if remove_result.returncode != 0 and worktree_dir.exists():
        shutil.rmtree(worktree_dir, ignore_errors=True)


def checkout_commit(worktree_dir: Path, commit: str) -> None:
    result = run_command(
        ["git", "-C", str(worktree_dir), "checkout", "--detach", "--force", commit],
        cwd=worktree_dir,
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or f"git checkout failed for {commit}")


def prepare_build_directory(worktree_dir: Path) -> None:
    ensure_clean_path(worktree_dir / "build")


def run_timed(
    command: Sequence[str],
    *,
    cwd: Path,
    env: Dict[str, str],
    log_prefix: Path,
) -> tuple[int, float]:
    start = time.monotonic()
    completed = subprocess.run(
        list(command),
        cwd=str(cwd),
        env=env,
        text=True,
        stdout=log_prefix.with_suffix(".stdout.log").open("w", encoding="utf-8"),
        stderr=log_prefix.with_suffix(".stderr.log").open("w", encoding="utf-8"),
        check=False,
    )
    elapsed = time.monotonic() - start
    return completed.returncode, elapsed


def csv_writer(csv_path: Path) -> tuple[csv.DictWriter, object]:
    csv_path.parent.mkdir(parents=True, exist_ok=True)
    file_exists = csv_path.exists()
    handle = csv_path.open("a", newline="", encoding="utf-8")
    fieldnames = [
        "position",
        "commit",
        "short_commit",
        "commit_date",
        "subject",
        "render_seconds",
        "compile_exit_code",
        "render_exit_code",
        "status",
    ]
    writer = csv.DictWriter(handle, fieldnames=fieldnames)
    if not file_exists:
        writer.writeheader()
    return writer, handle


def main() -> int:
    args = parse_args()
    repo = args.repo.resolve()
    output_csv = args.output_csv.resolve()
    worktree_dir = args.worktree_dir.resolve()
    extra_env = parse_env_pairs(args.env)

    commits = list_commits(repo, args.limit)
    if not commits:
        print("No commits found.", file=sys.stderr)
        return 1

    if args.dry_run:
        for index, commit in enumerate(commits, start=1):
            meta = commit_metadata(repo, commit)
            print(f"{index},{meta['short_commit']},{meta['commit_date']},{meta['subject']}")
        return 0

    processed = read_existing_commits(output_csv) if args.skip_existing else set()
    writer, handle = csv_writer(output_csv)

    try:
        setup_worktree(repo, worktree_dir, commits[0])

        for index, commit in enumerate(commits, start=1):
            if commit in processed:
                print(f"[skip] {index}/{len(commits)} {commit[:7]}")
                continue

            meta = commit_metadata(repo, commit)
            print(f"[run ] {index}/{len(commits)} {meta['short_commit']} {meta['subject']}")

            render_seconds = 0.0
            compile_exit_code = ""
            render_exit_code = ""
            status = "ok"

            try:
                checkout_commit(worktree_dir, commit)
                ensure_clean_path(worktree_dir / "output")
                prepare_build_directory(worktree_dir)

                env = os.environ.copy()
                env.update(extra_env)

                logs_dir = Path("/tmp/povCpp-perf-logs")
                logs_dir.mkdir(parents=True, exist_ok=True)
                compile_log_prefix = logs_dir / f"{index:03d}_{meta['short_commit']}_compile"
                render_log_prefix = logs_dir / f"{index:03d}_{meta['short_commit']}_render"

                compile_command = [str(worktree_dir / args.compile_command[0]), *args.compile_command[1:]]
                compile_exit_code, _ = run_timed(
                    compile_command,
                    cwd=worktree_dir,
                    env=env,
                    log_prefix=compile_log_prefix,
                )
                if compile_exit_code != 0:
                    status = "compile_failed"

                if status == "ok":
                    render_command = [str(worktree_dir / args.render_command[0]), *args.render_command[1:]]
                    render_exit_code, render_seconds = run_timed(
                        render_command,
                        cwd=worktree_dir,
                        env=env,
                        log_prefix=render_log_prefix,
                    )
                    if render_exit_code != 0:
                        status = "render_failed"
                else:
                    render_log_prefix.with_suffix(".stdout.log").write_text(
                        "Render skipped due to compilation failure.\n",
                        encoding="utf-8",
                    )
                    render_log_prefix.with_suffix(".stderr.log").write_text(
                        "",
                        encoding="utf-8",
                    )

            except Exception as exc:
                status = "script_error"
                render_log_prefix = Path("/tmp/povCpp-perf-logs") / f"{index:03d}_{meta['short_commit']}_render"
                render_log_prefix.with_suffix(".stderr.log").write_text(
                    f"{type(exc).__name__}: {exc}\n",
                    encoding="utf-8",
                )

            writer.writerow(
                {
                    "position": index,
                    **meta,
                    "render_seconds": f"{render_seconds:.6f}",
                    "compile_exit_code": compile_exit_code,
                    "render_exit_code": render_exit_code,
                    "status": status,
                }
            )
            handle.flush()

    finally:
        handle.close()
        if not args.keep_worktree:
            teardown_worktree(repo, worktree_dir)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
