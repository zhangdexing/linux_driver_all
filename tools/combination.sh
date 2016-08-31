
function combination_menu()
{
	cd $DBG_ROOT_PATH
	echo 组合选项:
	echo "[61] 完整编译lidbg并提交到产品二进制仓库"
	echo "[62] 编译basesystem并提交到产品二进制仓库"
	echo "[63] 编译lidbg和basesystem并提交到产品二进制仓库"
	echo "[64] 拷贝lidbg和basesystem到二进制仓库后生成升级包"

    	echo "[66] 编译拷贝lidbg或basesystem提交到测试分支并生成测试包"
    	echo "[67] 编译拷贝lidbg和basesystem并生成原生系统的升级包,拷贝到服务器"
        echo "[68] 完整编译BP并提交到二进制仓库"
	echo "[69] 编译拷贝lidbg或basesystem提交到远程分支并生成测试包"
	echo "[70] 编译烧写boot.img后重启"
}

function combination_handle()
{
	depository_request
	case $1 in
	61)
		read -p "输入提交到二进制仓库的说明文字：" descriptors
		lidbg_build_all && depository_clean && depository_pull && rm -rf $UPDATA_BIN_DIR/out && rm -rf $UPDATA_BIN_DIR/hw/* && depository_copy_lidbg  && depository_add_push "$descriptors" ;;
	62)
		read -p "输入提交到二进制仓库的说明文字：" descriptors
		cd $DBG_SYSTEM_DIR  && depository_clean && depository_pull && soc_build_release && depository_pull && depository_copy_basesystem && depository_add_push "$descriptors";;
	63)
		read -p "输入提交到二进制仓库的说明文字：" descriptors
		lidbg_build_all && cd $DBG_SYSTEM_DIR  && depository_clean && depository_pull && soc_build_release && depository_pull && rm -rf $UPDATA_BIN_DIR/out && rm -rf $UPDATA_BIN_DIR/hw/* && depository_copy_lidbg && depository_copy_basesystem && depository_add_push "$descriptors";;
	64)
	    depository_pull && rm -rf $UPDATA_BIN_DIR/out && rm -rf $UPDATA_BIN_DIR/hw/* && depository_copy_lidbg && depository_copy_basesystem && depository_make_package && gitk&;;
        66)
        echo "  编译选项如下"
        echo "  (1) 编译basesystem并拷贝到二进制仓库"
        echo "  (2) 编译lidbg并拷贝到二进制仓库"
        echo "  (3) 编译basesystem和lidbg并拷贝到二进制仓库"

        read -p "  输入编译选项：" which_combine
        read -p "  自定义提交到二进制仓库测试分支的名字：" branch_name
        read -p "  提交说明：" commit
        judge_which_combine && depository_make_package && new_remote_test_branch && new_branch_remove && copy_package_to_smb;; 
        #for test
        #judge_which_combine && new_remote_test_branch && new_branch_remove;; 
	67)
		read -p "  提交说明：" commit
		soc_build_origin_image && depository_copy_basesystem && depository_make_package && copy_package_to_smb;;
	68)
        echo  "start compile bp src"          
        build_all_handle
        echo "combile bp src over"                 
        read -p "输入提交到二进制仓库的说明文字：" descriptors
        depository_clean && depository_pull && depository_copy_bpfile && depository_add_push "$descriptors";; 
         69)
        echo "  编译选项如下"
        echo "  (1) 编译basesystem并拷贝到远程分支"
        echo "  (2) 编译lidbg并拷贝到远程分支"
        echo "  (3) 编译basesystem和lidbg并拷贝到远程分支"

        read -p "  输入编译选项：" which_combine
        read -p "  自定义提交到二进制仓库测试分支的名字：" branch_name
        read -p "  提交说明：" commit
	go_remote_test_branch && judge_which_combine_test && depository_make_package && remote_test_branch && new_branch_remove && copy_package_to_smb;;                
	70)
		soc_build_kernel && adb wait-for-devices reboot bootloader && soc_flash_kernel && sudo $DBG_TOOLS_PATH/fastboot reboot;;
	*)
		echo

	esac

	depository_release
}

