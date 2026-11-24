#!/usr/bin/env python3
"""Check the skill directory layout that makes the skills visible to every tool.

`.agents/skills/` holds the skills. Claude Code reads only `.claude/skills/`,
so each skill needs a symlink there, and `.github/skills` is a symlink kept
for older references. Also checks that every agent registry entry points at
its `agent-<short_name>` skill. Standard library only.
"""

import os
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SKILLS = ROOT / ".agents" / "skills"
CLAUDE = ROOT / ".claude" / "skills"
LEGACY = ROOT / ".github" / "skills"
REGISTRY = ROOT / ".github" / "agents" / "_shared" / "agent-registry.yml"


def main():
    problems = []
    names = sorted(p.name for p in SKILLS.iterdir() if p.is_dir())
    for name in names:
        skill_file = SKILLS / name / "SKILL.md"
        if not skill_file.is_file():
            problems.append(f"{name}: no SKILL.md")
            continue
        text = skill_file.read_text(encoding="utf-8")
        front = text.split("\n---", 1)[0] if text.startswith("---\n") else ""
        declared = re.search(r"^name: (\S+)$", front, re.M)
        if not declared or declared.group(1) != name:
            problems.append(f"{name}: frontmatter `name` must equal the directory name")
        link = CLAUDE / name
        target = f"../../.agents/skills/{name}"
        if not link.is_symlink() or os.readlink(link) != target:
            problems.append(f".claude/skills/{name}: must be a symlink to {target}")
    present = CLAUDE.iterdir() if CLAUDE.is_dir() else []
    extra = sorted(p.name for p in present if p.name not in names)
    problems.extend(f".claude/skills/{name}: no such skill" for name in extra)
    if not LEGACY.is_symlink() or os.readlink(LEGACY) != "../.agents/skills":
        problems.append(".github/skills: must be a symlink to ../.agents/skills")
    registry = REGISTRY.read_text(encoding="utf-8")
    entries = re.findall(
        r"^\s+short_name: (\S+)\n(?:.*\n)*?\s+path: (\S+)$", registry, re.M
    )
    if len(entries) != len(re.findall(r"^\s+short_name: ", registry, re.M)):
        problems.append("agent-registry.yml: every entry needs a `path`")
    for short_name, path in entries:
        expected = f".agents/skills/agent-{short_name}/SKILL.md"
        if path != expected or not (ROOT / path).is_file():
            problems.append(
                f"agent-registry.yml: {short_name}: path must be {expected}"
            )
    for problem in problems:
        print(problem, file=sys.stderr)
    if not problems:
        print(f"{len(names)} skills are laid out correctly.")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
