I am currently participating in an AtCoder Heuristic Contest, and I will use this generative AI to assist in developing my solution.

When using this generative AI, the "AtCoder Heuristic Contest Generative AI Usage Rules - Version 20250616" apply.

https://info.atcoder.jp/entry/ahc-llm-rules-en

Most importantly, after running the solution program, you must not modify or improve the solution, its approach, or its code based on the execution results unless the user gives a new explicit instruction to do so.

You may run the solution program and report its execution results, logs, scores, or other observations. After reporting them, you must stop and wait for a new instruction from the user before making any improvement based on those results.

Here, "solution program" refers to any program created or being created for the purpose of solving this contest problem, regardless of whether it was created by the user or by generative AI, and regardless of whether it is still in progress or already complete.

## Code maintenance

- Add and update Japanese comments as the implementation changes. Explain the purpose of each major phase, non-obvious evaluation formula, important invariant, and the reason for unusual processing order. Do not postpone all commenting until the end.
- Keep comments synchronized with the current implementation. When behavior or strategy changes, revise or remove stale comments in the same change.
- Remove code that has become unnecessary, including obsolete functions, constants, data structures, branches, diagnostics, and commented-out implementations. Do not keep abandoned experiments in the solution source merely for reference.
- Retain an old implementation only when it is still actively needed for a reproducible A/B comparison or another concrete purpose. In that case, document why it remains and how it is enabled.
- Before finishing a code change, check for unused or permanently disabled parts and remove everything that can be deleted safely. Compile with warnings enabled and verify behavior in proportion to the risk of the cleanup.

## Resource usage

- Minimize generative-AI usage. Do not spawn sub-agents unless the user explicitly requests their use. Work with the main agent by default.
- Read only the files and line ranges needed for the current task. Do not repeatedly load large source files, full histories, large logs, or whole diffs when `rg` and targeted ranges are sufficient.
- Keep tool output small. Avoid printing large generated datasets, complete benchmark logs, or unchanged parts of files into the conversation context.
- Do not perform redundant independent reviews or repeat an already completed verification unless new changes make it necessary.
- Prefer a short static inspection and a single focused check over broad exploratory work. If a broader investigation may consume substantial model context, first explain its scope and wait for the user's instruction.

## Execution ownership

- The user performs large executions. The agent must not run multi-seed benchmarks, large seed batches, corpus-wide collection, model training/evaluation over a corpus, exhaustive searches, or other long-running/high-CPU jobs.
- This restriction also applies when scripts and configurations for such an execution have been prepared. Stop after preparing and statically checking them, then provide the exact command for the user to run.
- The agent may compile and perform at most one minimal seed/case smoke test when necessary to check basic operation. Do not infer permission to expand that smoke test into a batch.
- Do not start a background or parallel benchmark job. Treat any multi-seed performance comparison, including a 100-seed paired comparison, as a user-run task.
- Only perform a larger execution if the user explicitly overrides this rule in a later instruction and identifies the specific run to execute.
