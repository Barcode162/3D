# Incident 2026-10-25: the models' textures stopped loading after J86

**Session:** [015cwLX4PmKxoTA3N6AscWbS](https://claude.ai/code/session_015cwLX4PmKxoTA3N6AscWbS)
**Errors counted:** 1
**Status:** closed

## What happened
J86.1 renamed the label the engine looks for in the phone versions of the
models (`image/x-bar3d-astc` → `image/x-looped-astc`, so no bar name stays
in the engine). The tool that makes those files (`tools/assets/astc_pack.py`)
still wrote the old label, so build `673494d` loaded every model without
its textures wherever the compressed versions are used (the iPhone among
them). The live check after the ship showed "texture N could not be
loaded" on every model.

## Impact
Live for a few minutes on `673494d`: the bar's models drawn without their
textures (flat colours) on phones.

## Root cause
The label is shared between the engine and a build tool; the rename was
done by searching the engine only. No check was run before shipping (the
user's rule, "The user tests"), and the live check ran after the push.

## Fix
J93: `astc_pack.py` writes the engine's label (`image/x-looped-astc`), with
a note naming where the engine checks it. Built and confirmed here: the
rebuilt models carry the new label.

## Proof
Every rename of a string in `looped/` is searched in the whole repo
(`grep -rn` over tools/, bar/, assets/ too) before it's saved; the live
check after every ship is read, not only its last line.
