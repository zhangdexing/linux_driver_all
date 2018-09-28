sudo chmod 777 /dev/ttyUSB*
ls /dev/ttyUSB* 
mv ~/minicom_log ~/minicom_log_$(date '+%y-%m-%d')
mkdir ~/minicom_log
nautilus ~/minicom_log

if [ -e "/dev/ttyUSB0"  ]; then
date > ~/minicom_log/ttyUSB0.txt 
gnome-terminal --title="ttyUSB0" -x minicom -D /dev/ttyUSB0 -b 115200 -C ~/minicom_log/ttyUSB0.txt
fi

if [ -e /dev/ttyUSB1  ]; then 
date > ~/minicom_log/ttyUSB1.txt 
gnome-terminal --title="ttyUSB1" -x minicom -D /dev/ttyUSB1 -b 115200 -C ~/minicom_log/ttyUSB1.txt
fi

if [ -e /dev/ttyUSB2  ]; then
date > ~/minicom_log/ttyUSB2.txt 
gnome-terminal --title="ttyUSB2" -x minicom -D /dev/ttyUSB2 -b 115200 -C ~/minicom_log/ttyUSB2.txt
fi

if [ -e /dev/ttyUSB3  ]; then
date > ~/minicom_log/ttyUSB3.txt
gnome-terminal --title="ttyUSB3" -x minicom -D /dev/ttyUSB3 -b 115200 -C ~/minicom_log/ttyUSB3.txt
fi

if [ -e /dev/ttyUSB4  ]; then
date > ~/minicom_log/ttyUSB4.txt 
gnome-terminal --title="ttyUSB4" -x minicom -D /dev/ttyUSB4 -b 115200 -C ~/minicom_log/ttyUSB4.txt
fi

if [ -e /dev/ttyUSB5  ]; then
date > ~/minicom_log/ttyUSB5.txt 
gnome-terminal --title="ttyUSB5" -x minicom -D /dev/ttyUSB5 -b 115200 -C ~/minicom_log/ttyUSB5.txt
fi

if [ -e /dev/ttyUSB6  ]; then
date > ~/minicom_log/ttyUSB6.txt 
gnome-terminal --title="ttyUSB6" -x minicom -D /dev/ttyUSB6 -b 115200 -C ~/minicom_log/ttyUSB6.txt
fi

if [ -e /dev/ttyUSB7  ]; then
date > ~/minicom_log/ttyUSB7.txt 
gnome-terminal --title="ttyUSB7" -x minicom -D /dev/ttyUSB7 -b 115200 -C ~/minicom_log/ttyUSB7.txt
fi

if [ -e /dev/ttyUSB8  ]; then
date > ~/minicom_log/ttyUSB8.txt
gnome-terminal --title="ttyUSB8" -x minicom -D /dev/ttyUSB8 -b 115200 -C ~/minicom_log/ttyUSB8.txt 
fi

if [ -e /dev/ttyUSB9  ]; then
date > ~/minicom_log/ttyUSB9.txt 
gnome-terminal --title="ttyUSB9" -x minicom -D /dev/ttyUSB9 -b 115200 -C ~/minicom_log/ttyUSB9.txt 
fi

if [ -e /dev/ttyUSB10  ]; then
date > ~/minicom_log/ttyUSB10.txt 
gnome-terminal --title="ttyUSB10" -x minicom -D /dev/ttyUSB10 -b 115200 -C ~/minicom_log/ttyUSB10.txt
fi

if [ -e /dev/ttyUSB11  ]; then
date > ~/minicom_log/ttyUSB11.txt 
gnome-terminal --title="ttyUSB11" -x minicom -D /dev/ttyUSB11 -b 115200 -C ~/minicom_log/ttyUSB11.txt 
fi

if [ -e /dev/ttyUSB12  ]; then
date > ~/minicom_log/ttyUSB12.txt 
gnome-terminal --title="ttyUSB12" -x minicom -D /dev/ttyUSB12 -b 115200 -C ~/minicom_log/ttyUSB12.txt 
fi

if [ -e /dev/ttyUSB13  ]; then
date > ~/minicom_log/ttyUSB13.txt 
gnome-terminal --title="ttyUSB13" -x minicom -D /dev/ttyUSB13 -b 115200 -C ~/minicom_log/ttyUSB13.txt 
fi

if [ -e /dev/ttyUSB14  ]; then
date > ~/minicom_log/ttyUSB14.txt 
gnome-terminal --title="ttyUSB14" -x minicom -D /dev/ttyUSB14 -b 115200 -C ~/minicom_log/ttyUSB14.txt 
fi

if [ -e /dev/ttyUSB15  ]; then
date > ~/minicom_log/ttyUSB15.txt 
gnome-terminal --title="ttyUSB15" -x minicom -D /dev/ttyUSB15 -b 115200 -C ~/minicom_log/ttyUSB15.txt 
fi

if [ -e /dev/ttyUSB16  ]; then
date > ~/minicom_log/ttyUSB16.txt 
gnome-terminal --title="ttyUSB16" -x minicom -D /dev/ttyUSB16 -b 115200 -C ~/minicom_log/ttyUSB16.txt 
fi


if [ -e /dev/ttyUSB17  ]; then
date > ~/minicom_log/ttyUSB17.txt 
gnome-terminal --title="ttyUSB17" -x minicom -D /dev/ttyUSB17 -b 115200 -C ~/minicom_log/ttyUSB17.txt 
fi

if [ -e /dev/ttyUSB18  ]; then
date > ~/minicom_log/ttyUSB18.txt 
gnome-terminal --title="ttyUSB18" -x minicom -D /dev/ttyUSB18 -b 115200 -C ~/minicom_log/ttyUSB18.txt 
fi

if [ -e /dev/ttyUSB19  ]; then
date > ~/minicom_log/ttyUSB19.txt 
gnome-terminal --title="ttyUSB19" -x minicom -D /dev/ttyUSB19 -b 115200 -C ~/minicom_log/ttyUSB19.txt 
fi

if [ -e /dev/ttyUSB20  ]; then
date > ~/minicom_log/ttyUSB20.txt 
gnome-terminal --title="ttyUSB20" -x minicom -D /dev/ttyUSB20 -b 115200 -C ~/minicom_log/ttyUSB20.txt 
fi



