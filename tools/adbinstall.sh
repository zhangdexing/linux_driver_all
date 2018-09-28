
#!bin/sh
times=0
for file in $1/*
do
    if test -f $file
    then
    let times++
        echo =========$times=============$file
        adb install $file
    fi
done
