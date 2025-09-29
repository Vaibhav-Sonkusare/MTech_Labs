#!/bin/bash

# Start the SSH agent
eval "$(ssh-agent -s)"

# Add your SSH key
ssh-add ~/.ssh/206125030_gitthub

# Navigate to your project directory
# cd ~/Documents/MTech_Labs || { echo "Directory not found"; exit 1; }

# Pull latest changes from the remote
# git pull origin main

# Add all changes, commit with a message provided as an argument or default message
# git add .
# commit_message=${1:-"Sync changes"}
# git commit -m "$commit_message"

read -p "Did you commit your changes? (y/n):" changes_commited

if [[ "$changes_commited" =~ ^[Yy]([Ee][Ss])?$ ]]; then
	echo "Pushing Changes to git..."
else
	echo "Add files to be commited using $ git add file1.name, file2.name..."
	echo "See changes staged to be commited using $ git status"
	echo "Commit the staged changes using $ git commit -m 'commit-message'"
	exit 1
fi

# Push changes to the remote
git push origin main

# Kill the SSH agent
eval "$(ssh-agent -k)"
