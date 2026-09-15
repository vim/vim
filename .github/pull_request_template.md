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

    patch 9.2.XXXX: short problem line (can be the same as Problem)
    Problem:  <one line: what is wrong, from the user's point of view>
    Solution: <one line: what this does about it>

Anything longer (mechanism, benchmark numbers, why an alternative was
rejected) goes in the body below the Solution line.

Keep the 9.2.XXXX, it will be filled in when merging

Align the Problem and Solution lines.

Changes to runtime files use the following form:

runtime(doc): for a documentation update
translation(isocode): translations into language
runtime(lang): runtime file changes for language lang
filetype: for changes to filetype detection
-->

    patch 9.2.XXXX: short problem description
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
closes:  #

### AI assistance

<!--
Please disclose AI involvement.

- If AI helped you write or investigate the change or test, add a line
  "Supported by AI" to the commit message.
- If AI wrote the commit itself, add the trailer
  "Co-authored-by: AI" instead.
-->

- [ ] AI involvement is disclosed in the commit message, or no AI was used

### Checklist

- [ ] The commit message follows the Problem/Solution form above
- [ ] `Signed-off-by:` trailer is present (`git commit -s`)
- [ ] A test was added, or the change cannot be tested (say why)
- [ ] The test fails without the change and passes with it
- [ ] Documentation under `runtime/doc/` was updated, if behaviour changed
- [ ] CI is green

### Anything reviewers should know

<!--
For example: a behaviour change users may notice, a platform you could
not test on, or an assumption the change relies on.
-->
