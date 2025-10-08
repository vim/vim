" Vim :syntime command


syntime on
syntime off
syntime clear
syntime report

syntime on | redraw! | syntime report

syntime on     | echo "..."
syntime on     " comment
syntime off    | echo "..."
syntime off    " comment
syntime clear  | echo "..."
syntime clear  " comment
syntime report | echo "..."
syntime report " comment


def Vim9Context()
  syntime on
  syntime off
  syntime clear
  syntime report

  syntime on | redraw! | syntime report

  syntime on     | echo "..."
  syntime on     # comment
  syntime off    | echo "..."
  syntime off    # comment
  syntime clear  | echo "..."
  syntime clear  # comment
  syntime report | echo "..."
  syntime report # comment
enddef

