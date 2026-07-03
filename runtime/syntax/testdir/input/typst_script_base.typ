#set document(title: "Sample Document")
#import "/typst/habamax.typ": *
#show: habamax.with(name: "Maxim Kim", email: "test@gmail.com")
#set heading(numbering: "(I)")
#show heading: set align(center)
#show heading: set text(font: "Arial")

#show "once?": it => [#it #it]

#show heading: it => block[
  \~
  #emph(it.body) // comment 1
  #counter(heading).display() // comment 2
  \~
]
