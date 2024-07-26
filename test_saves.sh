#!/bin/fish

function exists
	test -d $_flag_value
	or return
end

argparse --name=test_saves.sh 'i/interactive' 'd/save-dir=!exists' 's/success-file=?' 'f/fail-file=?' 'q/quiet' 'r/random=' 'n' 'loglevel=' 'limit=!_validate_int --min 0' -- $argv
or return

set -l save_dir $_flag_d
if not set -q _flag_s
	set _flag_s /dev/null
end
set -l success_file $_flag_s
if not set -q _flag_f
	set _flag_f /dev/null
end
set -l fail_file $_flag_f
if set -q _flag_r
	set _flag_r -r $_flag_r
end
if set -q _flag_n
	set _flag_n --fixed false
end
if set -q _flag_loglevel
	echo true
	set loglevel --loglevel $_flag_loglevel
else
	echo false
	set loglevel
end
if set -q _flag_limit
	echo true
	set limit --limit $_flag_limit
else
	echo false
	set limit
end

# all implemented saves
set -l saves 00150 10981 20176 21340 22280 30647 31904 32050 33762 40196 41427 42656 43786 50370 51781 52544 53897 60099 61212 62711 63534 70601 UNKNOWN NEXUS.11.711.2 NEXUS.12.534.4
#set -l saves 33762 40196 41427 42656 43786 50370 51781 52544 53897 60099 61212 62711 63534 70601 UNKNOWN NEXUS.11.711.2 NEXUS.12.534.4
# achievements only
#set -l saves 00150 21340 42656
# images only
#set -l saves 50370 51781 53897 70601 #NEXUS.11.711.2 NEXUS.12.534.4 NEXUS.13.370.9 NEXUS.14.781.3

set -l files_count 0
set -l success_count 0
set -l fail_count 0

echo > $success_file
echo > $fail_file

for id in $saves
	for s in $id.{0,1,2,C,N,I}.txt{,.bak,.bak2}
		set -l file $save_dir/$s
		if test -e $file
			set files_count (math $files_count + 1)
			# echo ./TIS-100-CXX $id $s
			# ./TIS-100-CXX $id $s
			echo ./TIS-100-CXX $_flag_q $_flag_r $_flag_n $loglevel $limit -c (basename $file) $id
			if ./TIS-100-CXX $_flag_q $_flag_r $_flag_n $loglevel $limit -c $file $id
				echo $s >> $success_file
				set success_count (math $success_count + 1)
			else 
				echo $s >> $fail_file
				set fail_count (math $fail_count + 1)
			end
			fgrep '##' $file
			egrep "# [0-9]+ / [0-9]+ / [0-9]+" $file
			if set -q _flag_i
				read _
			end
			echo
		end
	end
end

echo "$files_count saves tested. $success_count validated, $fail_count failed."
