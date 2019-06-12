#!/bin/bash

echo "List of clients:"

sentFiles=0
sentBytes=0
recFiles=0
recBytes=0
idsum=0
left=0
maxid=0
count=0

while read data; do

 set - $data

 if [[ "$#" -eq 3 ]]
 then
   type=$1
   filename=$2
   size=$3

   if [[ $type = "S" ]]
   then
	sentFiles=$(($sentFiles+1))
	sentBytes=$(($sentBytes+$size))
   else #type is R
	recFiles=$(($recFiles+1))
	recBytes=$(($recBytes+$size))
   fi 
 elif [[ "$#" -eq 1 ]] #id or Left
 then
   id=$1
   count=$(($count+1))
   if [[ $count -eq 1 ]] #init of minid
   then 
	minid=$id
   fi
   if [[ $id = "L" ]] #client has left
   then
	left=$(($left+1))
   else    #clients id
   	idsum=$(($idsum+1))
   	echo "$id"
	if [[ $id -gt maxid ]]
   	then
	  maxid=$id
	fi
	if [[ $id -lt minid ]]
        then
          minid=$id
        fi 
   fi
 fi
done

echo "Number of clients: $idsum"

echo "Number of sent files: $sentFiles"

echo "Number of received files: $recFiles"

echo "Number of sent bytes: $sentBytes"

echo "Number of received bytes: $recBytes"

echo "Number of left clients: $left"

echo "Maximum id: $maxid"

echo "Minimum id: $minid"

