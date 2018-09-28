
function judge_which_combine()
{
    echo $FUNCNAME
if [[ $which_combine -eq "1" ]];then
        
    echo "打包basesystem zip 包"
    cd $RELEASE_REPOSITORY
    soc_build_all && soc_make_otapackage && depository_clean && depository_pull && depository_copy_basesystem
#for test
#    echo "baseqcom.flb" > basesystem.txt
elif [[ $which_combine -eq "2" ]];then

    echo "打包 libdg zip 包"
    lidbg_build_all && depository_clean && depository_pull && depository_copy_lidbg
#for test
#    cd $RELEASE_REPOSITORY
#    echo "libdg.zip" > libdg.txt

elif [[ $which_combine -eq "3" ]];then
 
    echo "打包所有"
    depository_clean && depository_pull
    soc_build_all && soc_make_otapackage && depository_copy_basesystem
    lidbg_build_all && depository_copy_lidbg
#for test   
#    cd $RELEASE_REPOSITORY
#    echo "all.zip" > all.txt
else
    echo "输入错误"
    exit 
fi
}

function judge_which_combine_test()
{
    echo $FUNCNAME
if [[ $which_combine -eq "1" ]];then
        
    echo "打包basesystem zip 包"
    cd $RELEASE_REPOSITORY
    soc_build_all && soc_make_otapackage && depository_clean_test && depository_pull_test && depository_copy_basesystem_test
#for test
#    echo "baseqcom.flb" > basesystem.txt
elif [[ $which_combine -eq "2" ]];then

    echo "打包 libdg zip 包"
    lidbg_build_all && depository_clean_test && depository_pull_test && depository_copy_lidbg_test
#for test
#    cd $RELEASE_REPOSITORY
#    echo "libdg.zip" > libdg.txt

elif [[ $which_combine -eq "3" ]];then
 
    echo "打包所有"
    depository_clean_test && depository_pull_test
    soc_build_all && soc_make_otapackage && depository_copy_basesystem_test
    lidbg_build_all && depository_copy_lidbg_test
#for test   
#    cd $RELEASE_REPOSITORY
#    echo "all.zip" > all.txt
else
    echo "输入错误"
    exit 
fi
}

function new_remote_test_branch()
{
    echo $FUNCNAME
    cd $RELEASE_REPOSITORY
    git checkout -b $branch_name
    git add .
    git add -f $UPDATA_BIN_DIR
    git commit -am "$commit"
    expect $DBG_TOOLS_PATH/push $branch_name $DBG_REPO_PASSWORD
}
function go_remote_test_branch()
{
    echo $FUNCNAME
    cd $RELEASE_REPOSITORY
    expect $DBG_TOOLS_PATH/fetch $branch_name $DBG_REPO_PASSWORD
    git checkout -b $branch_name origin/$branch_name
   
}
function remote_test_branch()
{
    echo $FUNCNAME
    cd $RELEASE_REPOSITORY
    git add .
    git add -f $UPDATA_BIN_DIR
    git commit -am "$commit"
    expect $DBG_TOOLS_PATH/push $branch_name $DBG_REPO_PASSWORD
}

function new_branch_remove()
{
    echo $FUNCNAME
    cd $RELEASE_REPOSITORY
    git checkout $REPOSITORY_WORK_BRANCH
    git branch -D $branch_name
}


function copy_package_to_smb()
{

#	if [ ! -d /mnt/smb_$DBG_PLATFORM];then
	    mkdir /mnt/smb-$DBG_PLATFORM_ID-$DBG_PLATFORM
	  #  mount -t smbfs -o codepage=cp936,username=lisuwei,password=123456 $TEST_PACKAGE_PATH /mnt/smb-$DBG_PLATFORM_ID-$DBG_PLATFORM
#ubuntu 12.10 later cifs replace smbfs 
	    echo /mnt/smb-$DBG_PLATFORM_ID-$DBG_PLATFORM --> $TEST_PACKAGE_PATH 
	    mount -t cifs -o username=lisuwei,password=123456 $TEST_PACKAGE_PATH /mnt/smb-$DBG_PLATFORM_ID-$DBG_PLATFORM
#	fi
	mkdir /mnt/smb-$DBG_PLATFORM_ID-$DBG_PLATFORM/$commit
	cp -v $RELEASE_REPOSITORY/out/*.fup  /mnt/smb-$DBG_PLATFORM_ID-$DBG_PLATFORM/$commit/ && nautilus /mnt/smb-$DBG_PLATFORM_ID-$DBG_PLATFORM/$commit
	#rsync --progress $RELEASE_REPOSITORY/out/*.fup  /mnt/smb_$DBG_PLATFORM/$commit/ && nautilus /mnt/smb_$DBG_PLATFORM/$commit
}
