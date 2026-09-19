<!--
Thanks for contributing to Vim!

Please read CONTRIBUTING.md if you have not already.  The comments below
are not shown in the pull request; delete any section that does not apply.
-->

### What does this change do?

<!--
One or two sentences, please clearly write what problem this PR fixes,
for visual changes, a before/after screenshot is helpful
 -->

### Commit message

<!--
Changes to the C core are merged with a message in this form.

    Problem:  <one line: what is wrong, from the user's point of view>
    Solution: <one line: what this does about it>

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
    Solution: Solution line

<!--
If this PR references a previously merged one, add a references: #number
If this PR fixes a reported issue, add a fixes: #number
Keep the closes: header, it will be filled out when merging the PR

Align the # columns
-->
related: #number
fixes:   #number
closes:  #number

### AI assistance

<!--
Please disclose AI involvement by adding the trailer
  "Co-authored-by: AI tool".
-->

- [ ] AI involvement is disclosed in the commit message, or no AI was used

### Checklist

- [ ] The commit message follows the Problem/Solution form above
- [ ] `Signed-off-by:` trailer is present (`git commit -s`)
- [ ] A test was added, or the change cannot be tested (say why)
- [ ] A test was added and ran locally, or the change cannot be tested (say why), or is already tested.
- [ ] Documentation under `runtime/doc/` was updated

### Anything reviewers should know

<!--
For example: a behaviour change users may notice, a platform you could
not test on, or an assumption the change relies on.
-->
