#!/bin/bash
cd /flyapdata/install/
if [ -s /data/app/preinstall.txt ]; then  
    echo "3rddon't need to copy preinstall files" 
    echo "3rddon't need to copy preinstall files"  > /dev/dbg_msg
else
rm -rf /data/app/*.apk
apklist="$(ls *.apk)"  

for apkfile in ${apklist}; do  
#skip equal apk
if cmp  ${apkfile} /data/app/${apkfile} 
then
echo ==3rd=skip.copy===${apkfile} > /dev/dbg_msg
echo ==3rd=skip.copy===${apkfile} 
continue
fi
#copy
while [ ! -s /data/app/${apkfile} ]   
do 
echo ==3rd=copy===${apkfile} > /dev/dbg_msg
echo ==3rd=copy===${apkfile} 
dd if=${apkfile} of=/data/app/${apkfile}  
chmod 777 /data/app/${apkfile}
#for test
#rm -rf /data/app/${apkfile}  
done  
#break for loop
done 

echo appcheck > /data/app/preinstall.txt 
fi

