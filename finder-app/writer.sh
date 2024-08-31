#!/bin/sh

PATHFILE=$1
DIR=$( echo ${PATHFILE%/*} )    #extract only path name
FILE=$( echo ${PATHFILE##*/} )  #extract only file name
STR=$2

if [ ! $# -eq 2 ]
then
    echo Wrong number of parameters
    exit 1  #exit with error if there are no two arguments
fi

if [ -w ${PATHFILE} ]  #check if file exists and is writtable
then
    echo ${STR} > ${PATHFILE}    
else
    mkdir -p ${DIR} 2> /dev/null    #try to create the DIR and subdirs if necessary, without printing message
    if [ -w ${DIR} ]                #If we were able to create it, then we have also permission to create/write to file (on most cases)
    then
        echo ${STR} > ${PATHFILE}
    else
        echo Error, file could not be cretaed
        exit 1
    fi
fi


    
