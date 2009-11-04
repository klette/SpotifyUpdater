#!/bin/bash
# Needs seperate compiles updaters for now..

USERNAME=username
PASSWORD=pass

python fetch_lists.py

./updater-p3 $USERNAME $PASSWORD
while [ $? -ne 0 ]; do
	echo "Playlist not found.. Spotify sucks, so we try again.."
	sleep 5
	./updater-p3 $USERNAME $PASSWORD
done
./updater-mp3 $USERNAME $PASSWORD
while [ $? -ne 0 ]; do
	echo "Playlist not found.. Spotify sucks, so we try again.."
	sleep 5
	./updater-mp3 $USERNAME $PASSWORD
done
./updater-vg $USERNAME $PASSWORD
while [ $? -ne 0 ]; do
	echo "Playlist not found.. Spotify sucks, so we try again.."
	sleep 5
	./updater-vg $USERNAME $PASSWORD
done
./updater-radio1 $USERNAME $PASSWORD
while [ $? -ne 0 ]; do
	echo "Playlist not found.. Spotify sucks, so we try again.."
	sleep 5
	./updater-radio1 $USERNAME $PASSWORD
done
