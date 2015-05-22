source ../dbg_cfg.sh


case "$USERS_ID" in
   	0)
	export DBG_PLATFORM_PATH=/home/swlee/flyaudio
	export PATHJAVA1P6=/home/flyaudio/jdk1.6.0_31
	export PATHJAVA1P7=/home/flyaudio/java-7-openjdk-amd64
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
		8)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/rk3188/rk3188_rk3066_r-box_android4.4.2_sdk;;
		9)
		export DBG_SYSTEM_DIR=$DBG_PLATFORM_PATH/rk3188/px3_git;;
	esac;;
	
   	1)
	export DBG_PLATFORM_PATH=/home/wqrftf99/futengfei/work1_qucom
	export PATHJAVA1P6=/usr/local/jdk1.6.0_45
	export PATHJAVA1P7=/usr/local/java-7-openjdk-amd64
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
	esac;;

	2)
	export DBG_PLATFORM_PATH=/media/D/flyaudio/msm8226-5.0
	export PATHJAVA1P6=/usr/lib/java-6/jdk1.6.0_31
	export PATHJAVA1P7=/usr/lib/java-7/java-7-openjdk-amd64
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
	esac;;

	3)
	export DBG_PLATFORM_PATH=/home/w
	export PATHJAVA1P6=/usr/lib/jvm/jdk1.6.0_45
	export PATHJAVA1P7=/usr/lib/jvm/java-7-openjdk-amd64
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
	esac;;
        4)  
        export DBG_PLATFORM_PATH=/home/ctb/wu/
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
        esac;;
	 5)  
        export DBG_PLATFORM_PATH=/home2/chenchangsheng
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
        esac;;

esac


