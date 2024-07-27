#!/bin/fish

if set -q argv[1]
	set count $argv[1]
else
	set count (zenity --entry --text="Steps:")
end

# why does the focus keep going to the terminal?
# why do the keys not register half the time, and the other half of the time
# they get stuck?
if set -q $argv[2] && test $argv[2] -ne 0
	xdotool selectwindow \
		windowactivate --sync \
		windowfocus --sync \
		exec echo 1 \
		key --clearmodifiers --window %1 F6 \
		windowactivate --sync \
		windowfocus --sync \
		exec echo 2 \
		key --clearmodifiers --window %1 Escape \
		windowactivate --sync \
		windowfocus --sync \
		exec echo 3 \
		key --clearmodifiers --window %1 --repeat $count --delay 50 F6
	or return
else
	xdotool selectwindow \
		windowactivate --sync \
		windowfocus --sync \
		key --clearmodifiers --window %1 --repeat $count --delay 50 F6
end
