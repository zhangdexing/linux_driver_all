#!/bin/bash

echo "==3rd=in" 
echo "==3rd=in"  > /dev/dbg_msg

if [ -s /data/app/preinstall.txt ]; then  
    echo "==3rd=don't need to copy preinstall files" 
    echo "==3rd=don't need to copy preinstall files"  > /dev/dbg_msg
else

rm -rf /data/app/*.apk

#have a check.ensure flyapdata can be detect
while [ ! -s /data/app/preinstall.txt ]   
do 
echo ==3rd=check=== > /dev/dbg_msg
echo ==3rd=check===
cd /flyapdata/install/
dd if=preinstall.txt of=/data/app/preinstall.txt 
chmod 777 /data/app/preinstall.txt 
done  

#just for check
echo ==3rd=rm-preinstall.txt=== > /dev/dbg_msg
echo ==3rd=rm-preinstall.txt===
rm -rf /data/app/preinstall.txt 

echo "==3rd=start copy" 
echo "==3rd=start copy"  > /dev/dbg_msg
cd /flyapdata/install/
apklist="$(ls *.apk)"  

for apkfile in ${apklist}; do  
#skip equal apk
#if cmp  ${apkfile} /data/app/${apkfile} 
#then
#echo ==3rd=skip.copy===${apkfile} > /dev/dbg_msg
#echo ==3rd=skip.copy===${apkfile} 
#continue
#fi

#copy
while [ ! -s /data/app/${apkfile} ]   
do 
echo ==3rd=copy===${apkfile} > /dev/dbg_msg
echo ==3rd=copy===${apkfile} 
cd /flyapdata/install/
dd if=${apkfile} of=/data/app/${apkfile}  
chmod 777 /data/app/${apkfile}
#for test
#rm -rf /data/app/${apkfile}  
done  

#break for loop
done 


#success
while [ ! -s /data/app/preinstall.txt ]   
do 
echo ==3rd=success=== > /dev/dbg_msg
echo ==3rd=success===
cd /flyapdata/install/
dd if=preinstall.txt of=/data/app/preinstall.txt 
chmod 777 /data/app/preinstall.txt 
done  

#echo appcheck > /data/app/preinstall.txt
echo "==3rd=out" 
echo "==3rd=out"  > /dev/dbg_msg 

fi

