<!--
Thanks for contributing to Vim!

Please read CONTRIBUTING.md if you have not already.  The comments below
are not shown in the pull request; delete any section that does not apply.
-->

### What does this change do?

<!--
In one or two sentences, please clearly write what problem this PR fixes.
For visual changes, a before/after screenshot is helpful.
-->

### Commit message

<!--
Changes to the C core are merged with a message in this form.

    Problem:  <one short sentence: what is wrong, from the user's point of view>
    Solution: <one short sentence: what this does about it> (Your Name).

Anything longer (mechanism, benchmark numbers, why an alternative was
rejected) goes in the body below the Solution line.

Align the Problem and Solution lines.

Changes to runtime files use the following form:

runtime(doc): for a documentation update
translation(isocode): translations into language
runtime(lang): runtime file changes for language lang
filetype: for changes to filetype detection
-->

    Problem:  Problem line
    Solution: Solution line (Your Name).

<!--
If this PR references a previously merged one, add a related: #number
If this PR fixes a reported issue, add a fixes: #number
Keep the closes: header; it is filled in when the PR is merged

Align the # columns
-->
related: #number
fixes:   #number
closes:  #number

### AI assistance

<!--
Please disclose AI involvement by adding the trailer
  "Assisted-by: AI tool".
-->

- [ ] AI involvement is disclosed in the commit message, or no AI was used

### Checklist

- [ ] The commit message follows the Problem/Solution form above
- [ ] `Signed-off-by:` trailer is present (`git commit -s`), recommended but not required
- [ ] Tests were added, existing tests cover the change, or the change cannot be tested (say why)
- [ ] Documentation under `runtime/doc/` was updated, or no update is needed

### Anything reviewers should know

<!--
For example: a behavior change users may notice, a platform you could
not test on, or an assumption the change relies on.
-->
