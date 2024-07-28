#!/bin/fish

function exists
	test -d $_flag_value
	or return
end

argparse --ignore-unknown --name=retry_saves.sh 'l/retry-list=!test -f $_flag_value' 'i/interactive' 'd/save-dir=!exists' 's/success-file=?' 'f/fail-file=?' 'n' -- $argv
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
if set -q _flag_n
	set _flag_n --fixed false
end

set -l files_count 0
set -l success_count 0
set -l fail_count 0

echo > $success_file
echo > $fail_file

echo $saves

cat $_flag_l | while read -l s
	set -l id (echo $s | sed -Ee 's/(.*).[0-9A-Z].txt(.bak*)?/\1/')
	set -l file $save_dir/$s
	if test -e $file
		set files_count (math $files_count + 1)
		# echo ./TIS-100-CXX $id $s
		# ./TIS-100-CXX $id $s
		echo ./TIS-100-CXX $argv $_flag_n -c (basename $file) $id
		if ./TIS-100-CXX $argv $_flag_n -c $file $id
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

echo "$files_count saves tested. $success_count validated, $fail_count failed."
