# The Toolbelt

*System page · chapter: tools* · [all systems](../../generated/systems.md)

## What it is

The list of every tool and every project (`toolbelt/tools.json`); its page now opens Dev → Projects.

## Status

**Live** (as a list; `/toolbelt/` redirects to Dev → Projects).

## What you can do with it

Dev → Projects shows each project's tools: a tool with a page is a row to tap; one that asks for something (a report code) has its box; Claude's commands show as text.

## How it works

`toolbelt/tools.json`: `projects` (the one list, with site cards, J5.5), `groups`, `tools` (each with its project, group, page or command).

## Data

`toolbelt/tools.json`.

## Commands

A new tool: one entry in `tools` (the toolbelt test checks it).

## Limits

—

## Checks

`toolbelt` (quick).

## Known traps

A tool's `project` must exist in `projects`.

## History

R4.7 (2026-10-23): the Toolbelt became Dev → Projects.
