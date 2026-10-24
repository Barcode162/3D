# Changing the books, the tools, the structure

The structure (five books, the procedures' shape, job numbers, branches, the
plan's form) is meant never to need restructuring again. It grows by adding
in the places below; it changes only like this.

## Changing the structure itself
**When:** a book's job, the order of authority, the numbering, the branch
rules, the plan's form, the Charter's shape.
1. Write a decision record: `docs/records/decisions/<NNNN>-<slug>.md`
   (context, the decision, the options not taken, what it changes).
2. Ask the user (Charter §5: changing the structure is "ask first"): one
   question with the recommendation.
3. On a yes: change the books and tools in the same job; record the request.

## Adding things (no permission needed inside a job the user asked for)

| To add | Where | Then |
|---|---|---|
| A procedure | a new section or page in `docs/playbook/`, in the index | the shape: When · Before · Steps · Done when · If wrong · Records |
| A check | `tools/lib/checks.mjs` (CHECKS, and a RULES line for the files it covers) | a step in exactly one workflow (`tools/workflows_test.mjs` insists) |
| A tool | `tools/<name>.mjs` with its usage in its first lines; one line in `toolbelt/tools.json` | its Handbook entry (J6) |
| A system, page or data file | its code; its Handbook page (J6) | its checks |
| A project | `toolbelt/tools.json` → `projects` | Dev and the site list it |
| A word | `docs/charter/glossary.md` | before it's used anywhere |
| A known issue | `docs/records/known-issues.md`, with the job that fixes it | |
| A decision | `docs/records/decisions/` | |

## Never
- A second place for the same fact (link to it, or generate it with a check).
- Instructions outside the Playbook.
- Editing a generated block by hand (the Charter's rules block, `ui/themes.css`).
