source ../dbg_cfg.sh


case "$USERS_ID" in
   	0)
	export DBG_PLATFORM_PATH=/home/swlee/flyaudio
	export PATHJAVA1P6=/home/flyaudio/jdk1.6.0_31
	export PATHJAVA1P7=/home/flyaudio/java-7-openjdk-amd64
	export PATHJAVA1P8=/home/flyaudio/java-8-openjdk-amd64
	export WORK_REMOTE=0
	case "$DBG_PLATFORM_ID" in
	   	0)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625SSNSKQLYA10145451;;
	    	1)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625QSOSKQLYA3060-v2
		export RELEASE_REPOSITORY=/8x25q-release;;
	    	2)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/8228_bp_v103
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8226
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	3)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/msm8926-la-2-1-2_amss_qrd_no-l1-tds
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8926AAAAANLYD212005
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	4)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4120
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	6)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8626AAAAANLYD1431
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	7)
		export DBG_SYSTEM_DIR=/home/swlee/flyaudio/release/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
		8)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/rk3188/rk3188_rk3066_r-box_android4.4.2_sdk;;
		9)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/rk3188/px3_git
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
		10)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4120
		export BP_SOURCE_PATH=~/swlee/msm8626-la-1-0-4_amss_oem-xtra_no-l1-tds;;
		11)
		export DBG_SYSTEM_DIR=/home/msm/swlee/8909-5.1/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8909-release;;
	    	12)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/8228_bp_v103
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8226
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
                14)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
                15)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/imx6qdl-sabresd;;
                16)
		export DBG_SYSTEM_DIR=/home/msm/swlee/mt3561
		export RELEASE_REPOSITORY=/home/swlee/flyaudio/mt3561-release;;
                17)
		export DBG_SYSTEM_DIR=/home/msm/swlee/msm8996_6.0/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8996-release;;
                18)
		export DBG_SYSTEM_DIR=/home/msm/swlee/mt3561
		export RELEASE_REPOSITORY=/home/swlee/flyaudio/mt3561-release;;
                19)
		export DBG_SYSTEM_DIR=/home/msm/swlee/mt3561_clone2
		export RELEASE_REPOSITORY=/home/swlee/flyaudio/mt3561-release;;
                21)
		export DBG_SYSTEM_DIR=/home/msm/swlee/mt3561
		export RELEASE_REPOSITORY=/home/swlee/flyaudio/mt3561-release;;
                22)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8996-release;;
		23)
		export DBG_SYSTEM_DIR=/home/msm/swlee/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8909-release;;
	esac;;
	
   	1)
	export DBG_PLATFORM_PATH=/home/wqrftf99/futengfei/work1_qucom
	export PATHJAVA1P6=/usr/local/jdk1.6.0_45
	export PATHJAVA1P7=/usr/local/java-7-openjdk-amd64
	export PATHJAVA1P8=/usr/local/java-8-openjdk-amd64
	case "$DBG_PLATFORM_ID" in
	   	0)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625SSNSKQLYA10145451;;
	    	1)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625QSOSKQLYA3060-v2
		export RELEASE_REPOSITORY=/8x25q-release;;
	    	2)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/8228_bp_v103
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8226
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	3)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/msm8926-la-2-1-2_amss_qrd_no-l1-tds
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8926AAAAANLYD212005
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	4)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4120
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	6)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8626AAAAANLYD1431
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	7)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
		8)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/rk3188/rk3188_rk3066_r-box_android4.4.2_sdk;;
		9)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/rk3188/px3_git;;
		10)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4120
		export BP_SOURCE_PATH=~/swlee/msm8626-la-1-0-4_amss_oem-xtra_no-l1-tds;;
		11)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8909-release;;
	    	12)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/8228_bp_v103
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8226
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
                19)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/mt3561;;
	esac;;

	2)
	export DBG_PLATFORM_PATH=/home2/gaobenneng/msm8974
	export PATHJAVA1P6=/opt/JDK/jdk1.6.0_31
	export PATHJAVA1P7=/opt/JDK/java-7-openjdk-amd64
	export WORK_REMOTE=0
	export WORK_REMOTE_USERNAME=gaobenneng
	export WORK_REMOTE_PASSWORD=gaobenneng
	export WORK_REMOTE_PATH=/home2/gaobenneng/lidbg_qrd
	export WORK_LOCAL_PATH=/home/jesonjoy/workspace/lildbg_new/lidbg_qrd
	case "$DBG_PLATFORM_ID" in
			0)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625SSNSKQLYA10145451;;
			1)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625QSOSKQLYA3060-v2
		export RELEASE_REPOSITORY=/8x25q-release;;
			2)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/8228_bp_v103
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8226
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
			3)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/msm8926-la-2-1-2_amss_qrd_no-l1-tds
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8926AAAAANLYD212005
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
			4)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4120
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
			6)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8626AAAAANLYD1431
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	7)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
		8)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/rk3188/rk3188_rk3066_r-box_android4.4.2_sdk;;
		9)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/px3;;
		10)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4120
		export BP_SOURCE_PATH=~/swlee/msm8626-la-1-0-4_amss_oem-xtra_no-l1-tds;;
		11)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4275
		export BP_SOURCE_PATH=~/swlee/msm8626-la-1-0-4_amss_oem-xtra_no-l1-tds;;
	    	12)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/8228_bp_v103
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8226
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	esac;;

	3)
	export DBG_PLATFORM_PATH=~
	export PATHJAVA1P6=/usr/lib/jvm/jdk1.6.0_45
	export PATHJAVA1P7=/opt/JDK/java-7-openjdk-amd64
	export PATHJAVA1P8=/opt/JDK/java-8-openjdk-amd64
	export WORK_REMOTE=0
	export WORK_REMOTE_USERNAME=wuerwen
	export WORK_REMOTE_PASSWORD=wuerwen
	export WORK_REMOTE_PATH=/home2/wuerwen/driver/8909
	export WORK_LOCAL_PATH=/home/w/lidbg_qrd/
	case "$DBG_PLATFORM_ID" in
			0)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625SSNSKQLYA10145451;;
			1)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625QSOSKQLYA3060-v2
		export RELEASE_REPOSITORY=/8x25q-release;;
			2)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/8228_bp_v103
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8226
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
			3)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/msm8926-la-2-1-2_amss_qrd_no-l1-tds
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8926AAAAANLYD212005
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
			4)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4120
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
			6)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8626AAAAANLYD1431
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
                        7)
                export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/msm8974-la-4-0-2_amss_oem_l1-bin
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/8974
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
                        9)
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/px3
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/px3-release;;
			11)
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8909
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8909-release;;
			14)
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4275
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/8974-6.0/bp
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8909-release;;
			15)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/imx6qdl-sabresd;;
			16)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/mt3561
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/mt3561-release;;
			17)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/msm8996-release;;
			18)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/mt3561
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/mt3561-release;;
			20)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/tmp/mt3561
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/mt3561/mt3561-release;;
	esac;;

        4)  
        export DBG_PLATFORM_PATH=/home/ctb/wu
        export PATHJAVA1P6=/usr/lib/jvm/jdk1.6.0_45
        export PATHJAVA1P7=/opt/JDK/java-7-openjdk-amd64
	export PATHJAVA1P8=/opt/JDK/java-8-openjdk-amd64
        case "$DBG_PLATFORM_ID" in
                        0)  
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625SSNSKQLYA10145451;;
                        1)  
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625QSOSKQLYA3060-v2
                export RELEASE_REPOSITORY=/8x25q-release;;
                        2)  
                export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/8228_bp_v103
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8226
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
                        3)  
                export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/msm8926-la-2-1-2_amss_qrd_no-l1-tds
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8926AAAAANLYD212005
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
                        4)  
                export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4120
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
                        6)  
                export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8626AAAAANLYD1431
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
                       7)
                export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4275
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
			9)
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/px3
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/px3-release;;
	                11)
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8909
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8909-release;;
	                14)
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/8974-6.0/ap
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/8974-6.0/bp
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8909-release;;
			16)
		export DBG_SYSTEM_DIR=/home/w/mt3561;;
			17)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/820/msm8996
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/820/8996-release;;
			19)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/mt3561/
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH//mt3561-release;;
			22)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/src
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/820/8996-release;;
	 esac;;
	5) 
	export DBG_PLATFORM_PATH=/home/ccs/work
        export PATHJAVA1P6=/usr/lib/jvm/jdk1.6.0_45
        export PATHJAVA1P7=/opt/JDK/java-7-openjdk-amd64
        case "$DBG_PLATFORM_ID" in
                        0)  
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625SSNSKQLYA10145451;;
                        1)  
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625QSOSKQLYA3060-v2
                export RELEASE_REPOSITORY=/8x25q-release;;
                        2)  
                export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/8228_bp_v103
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8226
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
                        3)  
                export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/msm8926-la-2-1-2_amss_qrd_no-l1-tds
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8926AAAAANLYD212005
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
                        4)  
                export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4120
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
                        6)  
                export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8626AAAAANLYD1431
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
                       7)
                export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4275
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
		8)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/rk3188_rk3066_r-box_android4.4.2_sdk;;
		9)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/px3/px3;;
		10)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4120
		export BP_SOURCE_PATH=~/swlee/msm8626-la-1-0-4_amss_oem-xtra_no-l1-tds;;
		11)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4275
		export BP_SOURCE_PATH=~/lwy/msm8909-la-1-1_amss_oem_milestone-major;;
                15)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/Freescale-imx6;;
        esac;;
	6)
	export DBG_PLATFORM_PATH=/home2/luoweiye
	export PATHJAVA1P6=/opt/JDK/jdk1.6.0_31
	export PATHJAVA1P7=/opt/JDK/java-7-openjdk-amd64
	export WORK_REMOTE=1
	export WORK_REMOTE_USERNAME=luoweiye
	export WORK_REMOTE_PASSWORD=luoweiye
	export WORK_REMOTE_PATH=$DBG_PLATFORM_PATH/lidbg_qrd
	export WORK_LOCAL_PATH=/home/luo/Programs/SourceInsight/lidbg_qrd
	case "$DBG_PLATFORM_ID" in
	   	0)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625SSNSKQLYA10145451;;
	    	1)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625QSOSKQLYA3060-v2
		export RELEASE_REPOSITORY=/8x25q-release;;
	    	2)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/8228_bp_v103
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8226
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	3)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/msm8926-la-2-1-2_amss_qrd_no-l1-tds
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8926AAAAANLYD212005
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	4)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4120
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	6)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8626AAAAANLYD1431
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	7)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
		8)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/rk3188_rk3066_r-box_android4.4.2_sdk;;
		9)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/px3_git
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
		10)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4120
		export BP_SOURCE_PATH=~/swlee/msm8626-la-1-0-4_amss_oem-xtra_no-l1-tds;;
		11)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4275
		export BP_SOURCE_PATH=~/lwy/msm8909-la-1-1_amss_oem_milestone-major
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8909-release;;
		14)
        export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4275
        export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8909-release;;
		15)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/imx6qdl-sabresd
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/imx6q-release;;
		16)
		export DBG_SYSTEM_DIR=/home2/luoweiye/mt3561;;
	esac;;
	7)
	export DBG_PLATFORM_PATH=/home2/liangyihong/projDir
	export DBG_PLATFORM_PATH_BASIC=/home2/liangyihong/basicGit
	export DBG_PLATFORM_PATH_WWF=/home2/wuweifeng
	export DBG_PLATFORM_PATH_CCS=/home2/chenchangsheng
	export PATHJAVA1P6=/opt/JDK/jdk1.6.0_31
	export PATHJAVA1P7=/opt/JDK/java-7-openjdk-amd64
	case "$DBG_PLATFORM_ID" in
	   	0)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625SSNSKQLYA10145451;;
	    	1)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625QSOSKQLYA3060-v2
		export RELEASE_REPOSITORY=/8x25q-release;;
	    	2)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/8228_bp_v103
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8226
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/../8x26-release;;
	    	3)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/msm8926-la-2-1-2_amss_qrd_no-l1-tds
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8926AAAAANLYD212005
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	4)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4120
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	6)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8626AAAAANLYD1431
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	7)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
		8)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/rk3188/rk3188_rk3066_r-box_android4.4.2_sdk;;
		9)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/px3;;
		10)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4120
		export BP_SOURCE_PATH=~/swlee/msm8626-la-1-0-4_amss_oem-xtra_no-l1-tds;;
		11)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8909
		export BP_SOURCE_PATH=~/swlee/msm8626-la-1-0-4_amss_oem-xtra_no-l1-tds
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/../8909-release;;
		13)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH_BASIC/AllwinnerA80/android;;
		16)	
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/mt3561;;
		17)	
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8996
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8996-release;;
		19)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/mt3561/
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH//mt3561-release;;
		23)
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8909_6_0
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/8974-6.0/bp
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/../8909-release;;
		24)
                export DBG_SYSTEM_DIR=/home/jixiaohong/source/J6-Android-8.1.0;;
	esac;;
	8)
	export DBG_PLATFORM_PATH=/home/ctb/cks
	export PATHJAVA1P6=/opt/JDK/jdk1.6.0_31
	export PATHJAVA1P7=/opt/JDK/java-7-openjdk-amd64
	case "$DBG_PLATFORM_ID" in
	   	0)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625SSNSKQLYA10145451;;
	    	1)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625QSOSKQLYA3060-v2
		export RELEASE_REPOSITORY=/8x25q-release;;
	    	2)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/8228_bp_v103
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8226
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/../8x26-release;;
	    	3)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/msm8926-la-2-1-2_amss_qrd_no-l1-tds
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8926AAAAANLYD212005
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	4)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4120
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	6)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8626AAAAANLYD1431
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	7)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
		8)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/rk3188/rk3188_rk3066_r-box_android4.4.2_sdk;;
		9)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/px3;;
		10)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4120
		export BP_SOURCE_PATH=~/swlee/msm8626-la-1-0-4_amss_oem-xtra_no-l1-tds;;
		11)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8909
		export BP_SOURCE_PATH=~/swlee/msm8626-la-1-0-4_amss_oem-xtra_no-l1-tds
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/../8909-release;;
		13)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH_BASIC/AllwinnerA80/android;;
                15)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/imx6qdl-sabresd;;
	esac;;
	
	9)
	export DBG_PLATFORM_PATH=~
	export PATHJAVA1P6=/usr/lib/jvm/jdk1.6.0_45
	export PATHJAVA1P7=/opt/JDK/java-7-openjdk-amd64
	case "$DBG_PLATFORM_ID" in
			11)
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8909/M8974AAAAANLYD4275
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8909-release;;
			16)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/mt3561/mt3561
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/mt3561-release;;
			17)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8996/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/msm8996-release;;
			18)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/mt3561/mt3561
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/mt3561-release;;
			19)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/mt3561/mt3561
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/mt3561/mt3561-release;;
	esac;;
   	10)
	export DBG_PLATFORM_PATH=/home/jiangdepeng
	export PATHJAVA1P6=/opt/JDK/jdk1.6.0_31
	export PATHJAVA1P7=/opt/JDK/java-7-openjdk-amd64
	export PATHJAVA1P8=/opt/JDK/java-8-openjdk-amd64
	export WORK_REMOTE=0
	case "$DBG_PLATFORM_ID" in
	   	0)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625SSNSKQLYA10145451;;
	    	1)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/R8625QSOSKQLYA3060-v2
		export RELEASE_REPOSITORY=/8x25q-release;;
	    	2)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/8228_bp_v103
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8226
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	3)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/msm8926-la-2-1-2_amss_qrd_no-l1-tds
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8926AAAAANLYD212005
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	4)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4120
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	6)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/AMSS_M8974_40_R4120
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8626AAAAANLYD1431
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
	    	7)
		export DBG_SYSTEM_DIR=/home/swlee/flyaudio/release/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
		8)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/rk3188/rk3188_rk3066_r-box_android4.4.2_sdk;;
		9)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/rk3188/px3_git
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
		10)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4120
		export BP_SOURCE_PATH=~/swlee/msm8626-la-1-0-4_amss_oem-xtra_no-l1-tds;;
		11)
		export DBG_SYSTEM_DIR=/home/jiangdepeng/yangxiaofang/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8909-release;;
	    	12)
		export BP_SOURCE_PATH=$DBG_PLATFORM_PATH/8228_bp_v103
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8226
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
                14)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8x26-release;;
                15)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/imx6qdl-sabresd;;
                16)
		export DBG_SYSTEM_DIR=/home/msm/swlee/mt3561
		export RELEASE_REPOSITORY=/home/swlee/flyaudio/mt3561-release;;
                17)
		export DBG_SYSTEM_DIR=/home/msm/swlee/msm8996_6.0/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8996-release;;
                18)
		export DBG_SYSTEM_DIR=/home/msm/swlee/mt3561
		export RELEASE_REPOSITORY=/home/swlee/flyaudio/mt3561-release;;
                19)
		export DBG_SYSTEM_DIR=/home/msm/swlee/mt3561
		export RELEASE_REPOSITORY=/home/swlee/flyaudio/mt3561-release;;
                21)
		export DBG_SYSTEM_DIR=/home/msm/swlee/mt3561
		export RELEASE_REPOSITORY=/home/swlee/flyaudio/mt3561-release;;
                22)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8996-release;;
		23)
		export DBG_SYSTEM_DIR=/home/msm/swlee/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8909-release;;
	esac;;
	11)
	export DBG_PLATFORM_PATH=~
	export PATHJAVA1P6=/usr/lib/jvm/jdk1.6.0_45
	export PATHJAVA1P7=/opt/JDK/java-7-openjdk-amd64
	export PATHJAVA1P8=/opt/JDK/java-8-openjdk-amd64
	case "$DBG_PLATFORM_ID" in
			11)
                export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8909/M8974AAAAANLYD4275
                export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/8909-release;;
                15)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/imx6qdl-sabresd;;
			17)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/msm8996/M8974AAAAANLYD4275
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/msm8996-release;;
			19)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/mt3561
		export RELEASE_REPOSITORY=$DBG_PLATFORM_PATH/mt3561-release;;
	esac;;
	12)
	export DBG_PLATFORM_PATH=/home/jixiaohong
	case "$DBG_PLATFORM_ID" in
		24)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/source/J6-Android-8.1.0;;
	esac;;
	
esac



