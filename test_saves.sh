#!/bin/fish

function exists
	test -d $_flag_value
	or return
end

argparse --ignore-unknown --name=test_saves.sh 'i/interactive' 'd/save-dir=!exists' 's/success-file=?' 'f/fail-file=?' 'w/wrong-file=?' 'n' -- $argv
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
# wrong-file defaults to fail-file
if not set -q _flag_w
	set _flag_w $fail_file
end
set -l wrong_file $_flag_w

if set -q _flag_n
	set _flag_n --fixed 0
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
set -l wrong_count 0
set -l fail_count 0
set -l unmarked_count 0

echo > $success_file
echo > $fail_file
echo > $wrong_file

set tmp_result (mktemp)

function filter_result
	tail -n 1 $tmp_result | sed -Ee 's/^score: //' | sed "s,\x1B\[[0-9;]*[a-zA-Z],,g" | tr z c
end

for id in $saves
	for s in $id.{0,1,2,C,N,I}.txt{,.bak,.bak2}
		set -l file $save_dir/$s
		if test -e $file
			set files_count (math $files_count + 1)
			set -l expected (egrep "# [0-9]+ / [0-9]+ / [0-9]+" $file | sed -Ee 's@# ([0-9]+) / ([0-9]+) / ([0-9]+)@\1/\2/\3@')
			# echo ./TIS-100-CXX $id $s
			# ./TIS-100-CXX $id $s
			echo ./TIS-100-CXX $_flag_n $argv -c (basename $file) $id
			./TIS-100-CXX $_flag_n $argv -c $file $id | tee $tmp_result
			if test $pipestatus[1] -eq 0
				set -l result (filter_result)
				if test -z $expected 
					echo "$file; $result; //" >> $wrong_file
					set unmarked_count (math $unmarked_count + 1)
				else if test $result = $expected
					echo "$file; $result; $expected" >> $success_file
					set success_count (math $success_count + 1)
				else
					echo "$file; $result; $expected" >> $wrong_file
					set wrong_count (math $wrong_count + 1)
				end
			else
				set -l result (filter_result)
				if test -z $expected 
					echo "$file; $result; //" >> $wrong_file
				else 
					echo "$file; $result; $expected" >> $fail_file
				end
				set fail_count (math $fail_count + 1)
			end
			echo $expected
			fgrep '##' $file
			if set -q _flag_i
				read _
			end
			echo
		end
	end
end

echo "$files_count saves tested. $success_count validated, $unmarked_count passed with unknown validity, $wrong_count scored incorrectly, $fail_count failed."
rm $tmp_result

