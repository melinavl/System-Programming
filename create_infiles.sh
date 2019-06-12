#!/bin/bash

if [[ "$#" -ne 4 ]] #check if num of parameters is 4
then
  echo "Wrong Usage"
  exit 1
fi

dir_name=$1
num_of_files=$2
num_of_dirs=$3
levels=$4


if [[ ! -d "$dir_name" ]]
then
    mkdir -p -- "$dir_name"
    echo "File $dir_name created"
else
    echo "File $dir_name exists"
fi

declare -a directoryNames
declare -a directories
directories[0]=$dir_name
for (( i=0; i<$num_of_dirs; i++))
do
	len=$((1 + RANDOM % 8))
	directoryNames[i]=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w $len | head -n 1)
done

for ((i=0; i<$num_of_dirs; i++))
do
	cur_name=${directoryNames[i]}
	if ((i % levels == 0))
	then
		directories[i+1]=$dir_name;
		directories[i+1]="${directories[i+1]}/$cur_name"
	else
		previous=${directories[i]}
		directories[i+1]="$previous/$cur_name"
	fi
	
done

for ((i=0; i<=$num_of_dirs; i++))
do
	mkdir -p ${directories[i]}
done


declare -a fileNames
declare -a files

for (( i=0; i<$num_of_files; i++))
do
       len=$((1 + RANDOM % 8))
       fileNames[i]=$(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w $len | head -n 1)

done



for ((i=0; i<$num_of_files; i++))
do
	cur_name=${fileNames[i]}
       	files[i]="${directories[i % $(($num_of_dirs+1))]}/$cur_name"

        echo ${files[i]}        
done

for ((i=0; i<$num_of_files; i++))
do
	len=$((1024 + RANDOM % 128*1024))
	echo $(cat /dev/urandom | tr -dc 'a-zA-Z0-9' | fold -w $len | head -n 1) > ${files[i]}

done


