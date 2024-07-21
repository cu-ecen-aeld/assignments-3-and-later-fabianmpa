#!/bin/bash

FILESDIR=$1
SEARCHSTR=$2
NUMBEROFFILES=0
NUMBEROFMATCHES=0
WORKINGDIR=$( pwd ) #for testing purposes

touch tmp.txt
touch matches.txt

if [ ! $# -eq 2 ]
then
    echo Wrong number of parameters
    exit 1  #exit with error if there are no two arguments
fi

if [ ! -d ${FILESDIR} ]
then
    echo Directory does not exist
    exit 1 #exit with error if directory does not exist
fi

find ${FILESDIR} -mindepth 1 -type f > tmp.txt #look for all the files starting from working dir

while read FILE;
do
    cat ${FILE} | grep ${SEARCHSTR} >> matches.txt #look for matches on every file found in previous step
done < tmp.txt

NUMBEROFFILES=$( wc -l < tmp.txt )       #count how many files were found
NUMBEROFMATCHES=$( wc -l < matches.txt)  #count how many matches are

#remove the temporary files
rm tmp.txt
rm matches.txt

#corner case when there are no matches
if [ ${NUMBEROFMATCHES} -eq 0 ]
then 
    echo No matches found
    exit 1
fi

echo The number of files are ${NUMBEROFFILES} and the number of matching lines are ${NUMBEROFMATCHES}
