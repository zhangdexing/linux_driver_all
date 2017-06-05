#!/bin/bash
cd /flyapdata/preinstall/
if [ -s /data/app/preinstall.txt ]; then  
    echo "3rddon't need to copy preinstall files" 
    echo "3rddon't need to copy preinstall files"  > /dev/dbg_msg
else  
echo appcheck > /data/app/preinstall.txt 
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
echo ==3rd=copy===${apkfile} > /dev/dbg_msg
echo ==3rd=copy===${apkfile} 
dd if=${apkfile} of=/data/app/${apkfile}  
    chmod 777 /data/app/${apkfile}  
done 
fi

