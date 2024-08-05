#!/bin/fish

function exists
	test -d $_flag_value
	or return
end

argparse --ignore-unknown --name=test_saves.sh 'i/interactive' 'd/save-dir=!exists' 's/success-file=?' 'f/fail-file=?' 'w/wrong-file=?' 'a/all-file=?' 'n' -- $argv
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
if not set -q _flag_a
	set all_file /dev/null
else
	set all_file $_flag_a
end


set -l files_count 0
set -l success_count 0
set -l wrong_count 0
set -l fail_count 0

echo > $success_file
echo > $fail_file
echo > $wrong_file
echo > $all_file

set tmp_result (mktemp --suffix=TIS)

function filter_expected
	set -l RE "s@$id\.([0-9]+)-([0-9]+)-([0-9]+)-?(a?c?).txt@\1/\2/\3/\4@"
	basename $argv[1] | sed -Ee $RE -e 's@/$@@'
end

function filter_result
	tail -n 1 $tmp_result | sed -Ee 's,^score: ,,' -e 's, PR:.*,,' | sed "s,\x1B\[[0-9;]*[a-zA-Z],,g" | tr z c
end

function filter_pr
	tail -n 1 $tmp_result | grep 'PR:' | sed -Ee 's,(.*PR: ),,' | sed "s,\x1B\[[0-9;]*[a-zA-Z],,g"
end

for map in $save_dir/TIS*
	echo $map
	for id_folder in $map/*
		echo $id_folder
		set id (basename $id_folder)
		for file in $id_folder/$id*
			set files_count (math $files_count + 1)
			set -l expected (filter_expected $file)
			
			echo ./TIS-100-CXX $_flag_n $argv -c (basename $file) $id
			./TIS-100-CXX $_flag_n $argv -c $file $id | tee $tmp_result
			if test $pipestatus[1] -eq 0
				set -l result (filter_result)
				if test $result = $expected
					echo "$file; $result; $expected" >> $success_file
					set success_count (math $success_count + 1)
					echo $expected
				else
					echo "$file; $result; $expected" >> $wrong_file
					set wrong_count (math $wrong_count + 1)
					echo $expected '!'
				end
			else
				set -l result (filter_result)
				echo "$file; $result; $expected" >> $fail_file
				set fail_count (math $fail_count + 1)
				echo $expected '!'
			end
			set -l result (filter_result)
			set -l PR (filter_pr)
			echo "$file; $result; $expected; $PR" >> $all_file
			fgrep '##' $file
			if set -q _flag_i
				read _
			end
			echo
		end
	end
end

echo "$files_count saves tested. $success_count validated, $wrong_count scored incorrectly, $fail_count failed."
rm $tmp_result
