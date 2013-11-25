#!/bin/bash
# for file in `ls -p | grep "[^/]$"; do d2u $file; done

function list_directory()
{
	for file in `ls`
	do
		if test -d $file
		then
#			echo $file is directory.
			cd $file
			list_directory
			cd ..
		else
#			echo $file is file.
			if file $file | grep -q "text"
			then
#				echo "d2u $file"
				d2u $file
			fi
		fi
	done
}

list_directory
