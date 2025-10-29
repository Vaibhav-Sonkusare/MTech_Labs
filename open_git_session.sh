#!/bin/bash

echo "This program will open a session window (default 2 mins) to access git@github.com://vaibhav-sonksuare/MTech_Labs/"

cleanup() {
	echo -e "\nClosing session"
	eval "$(ssh-agent -k)"
	exit 1
}

trap cleanup SIGINT

# Start the SSH agent
eval "$(ssh-agent -s)"

# Add your SSH key
ssh-add ~/.ssh/206125030_gitthub

# Navigate to your project directory
# cd ~/Documents/MTech_Labs || { echo "Directory not found"; exit 1; }

session_open_time=20

# Get manual time requested by user
while getopts "t:" opt; do
	case $opt in
		t)
			session_open_time="$OPTARG"
			;;
		*)
			echo "Usage: $0 -t session_open_time_in_sec"
			exit 1
			;;
	esac
done

for ((i=1; i<=session_open_time; i++)); do
	echo -ne "Session open for $i/$session_open_time\r"
	sleep 1
done
echo ""

# Pull latest changes from the remote
# git pull origin main

# Add all changes, commit with a message provided as an argument or default message
# git add .
# commit_message=${1:-"Sync changes"}
# git commit -m "$commit_message"

# Push changes to the remote
# git push origin main

# Kill the SSH agent
eval "$(ssh-agent -k)"
