

proc UpdateBoard {layout turn} {
  global S system
  if {$system} {
    puts stdout "$layout $turn"
    flush stdout
  }
}

# Create canvas widget
canvas .c -width 300 -height 300 -bg white
pack .c -side top


# Frame to hold scrollbars
frame .b
pack  .b -expand 1 -fill x

###########################################################

proc DrawGameBoard {redraw} {
  global B S GameState
  set w 300
  set h 300
  set xy 0
  set x1 [expr {$w / 3.0}]
  set x2 [expr {2 * $w / 3.0}]
  set y1 [expr {$h / 3.0 }]
  set y2 [expr {2 * $h / 3.0}]
  set GameState "incomplete"

  if ($redraw) {
    .c delete all

    set B(0) [list 0 0 $x1 $y1]
    set B(1) [list $x1 0 $x2 $y1]
    set B(2) [list $x2 0 $w $y1]
    set B(3) [list 0 $y1 $x1 $y2]
    set B(4) [list $x1 $y1 $x2 $y2]
    set B(5) [list $x2 $y1 $w $y2]
    set B(6) [list 0 $y2 $x1 $h]
    set B(7) [list $x1 $y2 $x2 $h]
    set B(8) [list $x2 $y2 $w $h]

    for {set i 0} {$i < 9} {incr i} {
      .c create rect $B($i) -tag bd$i -fill {} -outline {}
      .c bind bd$i <Button-1> [list MakeBoard $i]
      #set B($i) [RescaleCell $B($i)]
    }

    for {set i 0} {$i < 9} {incr i} {
      set S($i) "U"
    }
  }

  .c create line $x1 0 $x1 $w -width 3
  .c create line $x2 0 $x2 $w -width 3
  .c create line 0 $y1 $h $y1 -width 3
  .c create line 0 $y2 $h $y2 -width 3
}
DrawGameBoard 1

###########################################################

button .b.sound -text "Sound"
button .b.resign -text "Resign" -command "Defeat"
button .b.exit -text "Exit" -command "ExitClient"
pack .b.sound .b.resign .b.exit -side left -expand 1 -fill x

canvas .g -width 300 -height 100 -bg darkgrey
pack .g -before .b

# .g create text 0 20 -anchor nw -text "You: (X)"
#.g create text 0 40 -anchor w -text "Opponent: (O)"
#.g create text 230 20 -anchor nw -text "Your move"

###########################################################

proc ExitClient {} {
  global GameState
  if {$GameState eq "complete"} {
    exit
  }
}

proc RescaleCell {xy} {
  foreach {x0 y0 x1 y1} $xy break
  set fifth [x0 * 3.0 / 5.0]
  return [list [expr {$x0+$fifth}] [expr {$y0+$fifth}] [expr {$x1-$fifth}] [expr {$y1-$fifth}]]
}

proc Defeat {} {
  global GameState system

  #if {$system eq "1"} {
    puts stdout "loss"
    flush stdout
    .g delete myturn
    .g create text 230 20 -anchor nw -text "You Lose"
    setGameState
  #}
}

proc YourMove {button} {
  if {$button eq "on"} {
    .g delete yourturn
    .g create text 230 20 -anchor nw -text "Your move" -tag myturn
    setSystem on
    bell
  }
  if {$button eq "off"} {
    .g delete myturn
    .g create text 140 20 -anchor nw -text "Awaiting Opponent Move" -tag yourturn
  }
  if {$button eq "clear"} {
    .g delete myturn
    .g delete yourturn
    setSystem off
  }
}

proc setSystem {state} {
  global system
  if {$state eq "on"} {
    set system 1
  }
  if {$state eq "off"} {
    set system 0
  }
}

proc setGameState {} {
  global GameState
  set GameState "complete"
}

proc SetSymbol {my_symbol} {
  global symbol

  set symbol $my_symbol
}

proc MakeBoard {loc} {
  global S symbol

  if {$S($loc) eq "U"} {
    set turn $loc
    set S($loc) $symbol
    set layout {}

    for {set i 0} {$i < 9} {incr i} {
      append layout $S($i)
    }
    UpdateBoard $layout $turn
  }
}

proc DrawMark {loc mark} {
  global B S symbol

    set S($loc) $mark
    if {$mark eq "O"} {
      .c create oval $B($loc) -width 3 -outline blue
    }
    if {$mark eq "X"} {
      foreach {x0 y0 x1 y1} $B($loc) break
      .c create line $x0 $y0 $x1 $y1 -width 3 -fill red
      .c create line $x0 $y1 $x1 $y0 -width 3 -fill red
    }
}

proc WinCondition {cond} {
  global B
  set w 300
  set h 300
  set half [expr {$w / 6.0}]
  set xy 0
  set xh1 [expr {$w / 3.0 - $half}]
  set xh2 [expr {2 * $w / 3.0 - $half}]
  set yh1 [expr {$h / 3.0 - $half}]
  set yh2 [expr {2 * $h / 3.0 - $half}]
  set xh3 [expr {$w - $half}]
  set yh3 [expr {$h - $half}]

  if {$cond eq "top"} {
    .c create line 0 $yh1 $w $yh1 -width 3 -fill green
  }
  if {$cond eq "mid"} {
    .c create line 0 $yh2 $w $yh2 -width 3 -fill green
  }
  if {$cond eq "bot"} {
    .c create line 0 $yh3 $w $yh3 -width 3 -fill green
  }
  if {$cond eq "dia"} {
    .c create line 0 0 $w $h -width 3 -fill green
  }
  if {$cond eq "lef"} {
    .c create line $xh1 0 $xh1 $h -width 3 -fill green
  }
  if {$cond eq "cen"} {
    .c create line $xh2 0 $xh2 $h -width 3 -fill green
  }
  if {$cond eq "rig"} {
    .c create line $xh3 0 $xh3 $h -width 3 -fill green
  }
  if {$cond eq "rev"} {
    .c create line 0 $h $w 0 -width 3 -fill green
  }
}

setSystem off