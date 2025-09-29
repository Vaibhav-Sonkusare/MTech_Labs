#!/bin/bash

# Start the SSH agent
eval "$(ssh-agent -s)"

# Add your SSH key
ssh-add ~/.ssh/206125030_gitthub

# Navigate to your project directory
# cd ~/Documents/MTech_Labs || { echo "Directory not found"; exit 1; }

# Pull latest changes from the remote
git pull origin main

# Add all changes, commit with a message provided as an argument or default message
# git add .
# commit_message=${1:-"Sync changes"}
# git commit -m "$commit_message"

# Push changes to the remote
# git push origin main

# Kill the SSH agent
eval "$(ssh-agent -k)"
