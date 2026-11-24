---
name: agent-skill-authoring
description: Use when adding or modifying rules, finding classes, or agent definitions in the F Prime multi-agent review system. Ensures changes are concise, consistent, and correctly wired across all files.
---

# Skill: Authoring concise agent and skill updates

Guidelines for adding or modifying rules, agents, and skills in the
F Prime multi-agent review system. The goal is minimal, consistent
changes that AI reviewers can interpret without verbose explanation.

---

## 0. Where skills and agents live

Skills follow the [Agent Skills specification](https://agentskills.io/specification):
one directory per skill, holding a file named exactly `SKILL.md`.

| Artifact | Path |
|---|---|
| Skill | `.agents/skills/<skill-name>/SKILL.md` |
| Agent | `.agents/skills/agent-<agent-name>/SKILL.md` |
| Generated agent files (never hand-edited) | `.github/agents/<agent-name>.agent.md`, `.claude/agents/<agent-name>.md` |
| Shared review contract / registry / phase procedures (e.g. `phase-review-loop.md`) | `.github/agents/_shared/` |

The directory name must equal the `name` in the skill's frontmatter,
using lowercase letters, digits, and hyphens only — a mismatch makes
the skill fail to load silently. Frontmatter must carry `name` and a
`description` that states when to use the skill; optional fields such
as `argument-hint` and `triggers` are permitted. Any supporting file
the skill needs (scripts, templates) goes beside its `SKILL.md`.

`.agents/skills/` is the one real location; Copilot, VS Code, Codex,
Cursor, Gemini CLI, and Devin scan it directly. Claude Code scans only
`.claude/skills/`, so every skill also needs a symlink there
(`ln -s ../../.agents/skills/<skill-name> .claude/skills/<skill-name>`).
CI runs `python3 .agents/tools/check_skill_layout.py`; run it after
adding a skill. `.claude/agents/` and `.github/agents/` (outside
`_shared/`) hold only generated files; keep personal agents elsewhere.
`.github/skills` is a symlink kept for older references in a local
checkout; it does not resolve in GitHub URLs or API reads. Never add
files through it. Cite a skill by its full path
(`.agents/skills/pr-diff-scoping/SKILL.md`) where the reader is being
sent to it as a source of truth, and by bare name (`pr-diff-scoping`)
in prose, tables, and diagrams where the pointer is incidental.

---

## 1. Writing rules (fprime-cpp-design and similar)

**Be terse.** The rule title carries most of the meaning. AI agents
are capable of inferring rationale from a well-named rule. Aim for
2-6 lines of prose per rule — comparable to CPP-6 or CPP-15, not
CPP-1 or CPP-25 (which are longer because they have genuinely
complex scoping).

Checklist for a new rule:

- **Title** (`#### CPP-N — <imperative phrase>`): self-explanatory
  in isolation. A reader should understand the rule from the title
  alone.
- **Body**: 2-6 lines. State what is required or forbidden, one
  exception if any, and a cross-reference to a related rule if
  relevant. Omit motivation the title already conveys.
- **Code examples**: include only when the correct pattern is
  non-obvious. One "avoid / prefer" pair, max 6 lines each. Skip
  examples when the rule is self-evident from the title (e.g.,
  "use `nullptr`", "mark overrides with `override`").
- **Finding-class row** (§2 table): one row, stable string name.
  Use the `cpp-<description>` naming convention.
- **Triage hint** (§3): one sub-bullet under the appropriate
  cluster. State default tag and upgrade/downgrade condition.

**Anti-patterns to avoid:**

- Restating the title in the body ("This rule requires X" when the
  title already says "Require X").
- Multi-paragraph rationale — put rationale in the PR description,
  not the rule text.
- Multiple code examples showing the same point.

## 2. Updating agent skills (e.g., agent-fprime-code-review)

An agent is a skill named `agent-<name>`. Its frontmatter is `name`, a
double-quoted one-line `description`, and a `metadata` block with
exactly `agent-name`, `agent-tools` (space-separated subset of `read
search edit execute`), `agent-user-invocable` and
`agent-model-invocable` (`"true"` / `"false"`). After adding an agent or
changing its frontmatter, run
`python3 .agents/tools/generate_agent_wrappers.py` and commit the
regenerated Copilot and Claude Code files; CI fails when they are stale.

Agent skills reference the rule skill as the source of truth. When adding
a rule to a skill, update the agent skill with **minimal additions**:

- **Scope range**: bump "CPP-1 through CPP-N" to the new max.
- **Finding-class list**: add one line for the new finding class.
- **Heuristics**: append a short clause ("; <pattern> (CPP-N)") to
  the relevant scan-order item. Do not duplicate the rule body.
- **Triage rules of thumb**: append a short clause to the relevant
  cluster bullet. Mirror the skill's §3 hint verbatim or shorter.

**Do not** re-explain the rule in the agent skill. The rule skill is the
source of truth; the agent skill is a lookup index.

## 3. Updating the registry (agent-registry.yml)

Bump the scope range in `scope_summary` (e.g., "CPP-33" → "CPP-34").
No other changes needed for a new rule — the registry describes
agent scope, not individual rules.

## 4. Prefer linking existing documentation

Before writing detailed procedural content in a skill, **search the
repo for existing docs** (`docs/`, README files, how-to guides) that
already cover the topic. Skills should provide:

- **When / why** to use a technique (decision criteria, trade-offs).
- **Brief orientation** of core constructs (a few bullet points).
- **A link** to the canonical doc for the full procedure and examples.

**Do not** reproduce scaffolding steps, file layouts, code templates,
or implementation walkthroughs that already exist in a doc. Duplicated
content drifts out of sync with the source and risks hallucination
when the skill is used as training context.

A good pattern:

```md
Scaffold with `fprime-util new --rule-based-test`. For file layout,
implementation patterns, and full examples see the
[rules-based testing guide](https://github.com/nasa/fprime/blob/devel/docs/how-to/rule-based-testing.md).
```

A bad pattern: copy-pasting the entire guide into the skill.

When no existing doc covers the topic, the skill *is* the primary
source — write the detail there. But flag it for future extraction
into a standalone doc if the content grows beyond ~40 lines of
procedural steps.

## 5. Cross-file consistency checklist

Before committing a new rule, verify all references are updated:

| Location | What to update |
|---|---|
| `fprime-cpp-design` §1 | Rule text |
| `fprime-cpp-design` §2 | Finding-class row |
| `fprime-cpp-design` §3 | Triage hint + cluster header |
| `agent-fprime-code-review` scope | "CPP-1 through CPP-N" (2 occurrences) |
| `agent-fprime-code-review` finding classes | New entry |
| `agent-fprime-code-review` heuristics | Clause in scan item |
| `agent-fprime-code-review` triage | Clause in cluster bullet |
| `agent-registry.yml` | `scope_summary` range |
