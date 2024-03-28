#!/bin/bash

# Check if a file is provided as an argument - *do not include the extension*
if [ -z "$1" ]; then
    echo "Usage: $0 <input_file>"
    exit 1
fi

input_file="$1.txt"
echo $input_file
output_dir="new"  # Default output directory is "cleaned"

# Check how many lines it has
line_count="cat $input_file | wc -l"
echo $line_count
# How many times to repeat the code
line_count_repeat=$line_count/400
echo $line_count_repeat
line_count_up=0

if ["$line_count_repeat" -lt "1"]; then
    # Make a numbered copy and run a check for the same name
    output_file="$1"
    if [[ -e $output_file.txt || -L $output_file.txt ]] ; then
        i=0
        while [[ -e $output_file-$i.txt || -L $output_file-$i.txt ]] ; do
            let i++
        done
        # redeclare output_file as the new numbered copy
        output_file="$output_file-$i.txt"
        echo $output_file
    fi
    # create the output file
    touch -- $output_file
    line_count_repeat=$line_count_repeat - 1
    echo $line_count_repeat

    # Copy the next 400 lines and save the file
    line_start=$line_count_up * 400
    line_end=$line_start + 400
    sed -n "$line_start,$line_end'p'" $input_file > $output_file
    echo Copy number $line_count_repeat done!
    line_count_up=$line_count_up + 1
 fi

