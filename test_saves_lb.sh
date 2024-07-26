#!/bin/fish

function exists
	test -d $_flag_value
	or return
end

argparse --name=test_saves.sh 'i/interactive' 'd/save-dir=!exists' 's/success-file=?' 'f/fail-file=?' 'q/quiet' 'r/random=' 'n' -- $argv
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

# set -l saves 00150 10981 20176 21340 22280 30647 31904 32050 33762 40196 41427 42656 43786 50370 51781 52544 53897 60099 61212 62711 63534 70601 UNKNOWN
#set -l saves 33762 40196 41427 42656 43786 50370 51781 52544 53897 60099 61212 62711 63534 70601 UNKNOWN NEXUS.11.711.2 NEXUS.12.534.4
# achievements only
#set -l saves 00150 21340 42656
# images only
set -l saves 50370 51781 53897 70601 #NEXUS.11.711.2 NEXUS.12.534.4 NEXUS.13.370.9 NEXUS.14.781.3

set -l files_count 0
set -l success_count 0
set -l fail_count 0

echo > $success_file
echo > $fail_file

for id in $saves
	for file in $save_dir/$id/$id*
		set files_count (math $files_count + 1)
		echo ./TIS-100-CXX $_flag_q $_flag_r $_flag_n -c (basename $file) $id
		if ./TIS-100-CXX $_flag_q $_flag_r $_flag_n -c $file $id
			echo $file >> $success_file
			set success_count (math $success_count + 1)
		else 
			echo $file >> $fail_file
			set fail_count (math $fail_count + 1)
		end
		
		set -l RE "s@$id\.([0-9]+)-([0-9]+)-([0-9]+)-?(a?c?).txt@\1/\2/\3/\4@"
		echo (basename $file | sed -Ee $RE | sed -Ee 's@/$@@')
		fgrep '##' $file
		if set -q _flag_i
			read _
		end
		echo
	end
end

echo "$files_count saves tested. $success_count validated, $fail_count failed."