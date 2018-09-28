
filelist=`ls $DBG_OUT_PATH`
main_loop_times=0


if [ -f $DBG_KERNEL_OBJ_DIR/signing_key.priv ];
then
echo "sign file exist"
else
echo "sign file miss,return"
return 
fi


cd $DBG_KERNEL_SRC_DIR
for file in $filelist
do 
if [[ "${file##*.}" = "ko" ]]; then
 echo sign_modules.$main_loop_times [$file]
scripts/sign-file sha512 $DBG_KERNEL_OBJ_DIR/signing_key.priv $DBG_KERNEL_OBJ_DIR/signing_key.x509 $DBG_OUT_PATH/$file
let main_loop_times++
fi
done


