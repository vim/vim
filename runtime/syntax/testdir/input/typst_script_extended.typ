#let habamax(name: "", email: "", text_size: 14pt, doc) = {
  import "@preview/cheq:0.3.0": checklist
  show: checklist

  set text(size: text_size, lang: "en")

  set page(
    header: context {
      if counter(page).get().first() > 1 {
        set align(right)
        set text(8pt)
        smallcaps(document.title)
      }
    },
    footer: context {
      if counter(page).final().first() > 1 {
        set align(right)
        set text(8pt)
        counter(page).display("1 / 1", both: true)
      }
    },
  )
  // set par(
  //   justify: true,
  //   first-line-indent: 2em,
  //   spacing: 0.65em)
  set par(justify: true)
  set heading(numbering: "1.")
  show title: set align(center)
  show title: smallcaps
  if name == "" {
    show title: set block(below: 2em)
    title()
  } else {
    title()
    pad(bottom: 2em,
    align(center)[
      #name
      #h(5pt)
      #if email != "" {
        [( #link("mailto:" + email) )]
      }
    ])
  }

  show raw.where(block: true): it => block(
    stroke: (left: 1.5pt + luma(220)),
    inset: 10pt,
    radius: 6pt,
    width: 100%,
    it,
  )

  doc
}
