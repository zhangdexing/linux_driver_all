sudo chmod 777 /dev/ttyUSB*
ls /dev/ttyUSB* 
mv ~/minicom_log ~/minicom_log_$(date '+%y-%m-%d_%H-%M-%S')
mkdir ~/minicom_log
# mt3561 921600
# tmux attach -t logcatch
# tmux kill-session -t logcatch

tmux new-session -s logcatch -n ctrl -d
tmux send-keys -t logcatch 'cd ~/minicom_log' C-m


if [ -e "/dev/ttyUSB0"  ]; then
tmux new-window -n ttyUSB0-1 -t logcatch
tmux split-window -h -t logcatch:1
tmux send-keys -t logcatch:1.0 'date > ~/minicom_log/ttyUSB0.txt && minicom -D /dev/ttyUSB0 -b 115200 -C ~/minicom_log/ttyUSB0.txt' C-m
tmux send-keys -t logcatch:1.1 'date > ~/minicom_log/ttyUSB1.txt && minicom -D /dev/ttyUSB1 -b 115200 -C ~/minicom_log/ttyUSB1.txt' C-m
tmux send-keys -t logcatch:1.0 C-a 'z' 'n'
tmux send-keys -t logcatch:1.1 C-a 'z' 'n'
fi


if [ -e "/dev/ttyUSB2"  ]; then
tmux new-window -n ttyUSB2-3 -t logcatch
tmux split-window -h -t logcatch:2
tmux send-keys -t logcatch:2.0 'date > ~/minicom_log/ttyUSB2.txt && minicom -D /dev/ttyUSB2 -b 115200 -C ~/minicom_log/ttyUSB2.txt' C-m
tmux send-keys -t logcatch:2.1 'date > ~/minicom_log/ttyUSB3.txt && minicom -D /dev/ttyUSB3 -b 115200 -C ~/minicom_log/ttyUSB3.txt' C-m
tmux send-keys -t logcatch:2.0 C-a 'z' 'n'
tmux send-keys -t logcatch:2.1 C-a 'z' 'n'
fi

if [ -e "/dev/ttyUSB4"  ]; then
tmux new-window -n ttyUSB4-5 -t logcatch
tmux split-window -h -t logcatch:3
tmux send-keys -t logcatch:3.0 'date > ~/minicom_log/ttyUSB4.txt && minicom -D /dev/ttyUSB4 -b 115200 -C ~/minicom_log/ttyUSB4.txt' C-m
tmux send-keys -t logcatch:3.1 'date > ~/minicom_log/ttyUSB5.txt && minicom -D /dev/ttyUSB5 -b 115200 -C ~/minicom_log/ttyUSB5.txt' C-m
tmux send-keys -t logcatch:3.0 C-a 'z' 'n'
tmux send-keys -t logcatch:3.1 C-a 'z' 'n'
fi

if [ -e "/dev/ttyUSB6"  ]; then
tmux new-window -n ttyUSB6-7 -t logcatch
tmux split-window -h -t logcatch:4
tmux send-keys -t logcatch:4.0 'date > ~/minicom_log/ttyUSB6.txt && minicom -D /dev/ttyUSB6 -b 115200 -C ~/minicom_log/ttyUSB6.txt' C-m
tmux send-keys -t logcatch:4.1 'date > ~/minicom_log/ttyUSB7.txt && minicom -D /dev/ttyUSB7 -b 115200 -C ~/minicom_log/ttyUSB7.txt' C-m
tmux send-keys -t logcatch:4.0 C-a 'z' 'n'
tmux send-keys -t logcatch:4.1 C-a 'z' 'n'
fi

if [ -e "/dev/ttyUSB8"  ]; then
tmux new-window -n ttyUSB8-9 -t logcatch
tmux split-window -h -t logcatch:5
tmux send-keys -t logcatch:5.0 'date > ~/minicom_log/ttyUSB8.txt && minicom -D /dev/ttyUSB8 -b 115200 -C ~/minicom_log/ttyUSB8.txt' C-m
tmux send-keys -t logcatch:5.1 'date > ~/minicom_log/ttyUSB9.txt && minicom -D /dev/ttyUSB9 -b 115200 -C ~/minicom_log/ttyUSB9.txt' C-m
tmux send-keys -t logcatch:5.0 C-a 'z' 'n'
tmux send-keys -t logcatch:5.1 C-a 'z' 'n'
fi


if [ -e "/dev/ttyUSB10"  ]; then
tmux new-window -n ttyUSB10-11 -t logcatch
tmux split-window -h -t logcatch:6
tmux send-keys -t logcatch:6.0 'date > ~/minicom_log/ttyUSB10.txt && minicom -D /dev/ttyUSB10 -b 115200 -C ~/minicom_log/ttyUSB10.txt' C-m
tmux send-keys -t logcatch:6.1 'date > ~/minicom_log/ttyUSB11.txt && minicom -D /dev/ttyUSB11 -b 115200 -C ~/minicom_log/ttyUSB11.txt' C-m
tmux send-keys -t logcatch:6.0 C-a 'z' 'n'
tmux send-keys -t logcatch:6.1 C-a 'z' 'n'
fi

tmux attach -t logcatch

