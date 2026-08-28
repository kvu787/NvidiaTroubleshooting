# External tools

If necessary, you can use the tools in `C:\Users\k\Program`.

If necessary, you can refer to local copies of source repos in `C:\Users\k\Repository\External`.

# Conversations

Record and commit all conversations in a `Conversations` folder that is located at the root of this Git repo.
Use one file per conversation.
Prefix these commits with `[record]`.
If I attach images to prompts, you must also save and record these in the conversation logs.

# Godot

If you create a Godot project, include a "Run.cmd" file that builds and launches the standalone exe of the Godot project by double clicking the Run.cmd from File Explorer.

# Git

When implementing stuff, avoid difficult-to-review "mega-commits". Split large work into multiple commits to make it easier to review.
Separate commits that record conversations from other commits.

# Mathematical notation in Markdown

Any mathematical notation in Markdown files (LaTeX, KaTeX, MathJax, etc) must be compatible with VSCode's Markdown previewer and GitHub's Markdown displayer.

# Compatibility

Do not attempt to maintain any sort of application compatibility between different commits of the repo. This creates unwanted complexity.
