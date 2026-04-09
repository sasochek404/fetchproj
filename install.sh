#!/bin/sh
set -e

cc -o2 main.c -o fetchproj
sudo mv fetchproj /usr/local/bin/fetchproj
chmod +x /usr/local/bin/fetchproj

cat << "EOF"
 ___              _           _  _            _  _
|_ _| _ __   ___ | |_   __ _ | || |  ___   __| || |
 | | | '_ \ / __|| __| / _` || || | / _ \ / _` || |
 | | | | | |\__ \| |_ | (_| || || ||  __/| (_| ||_|
|___||_| |_||___/ \__| \__,_||_||_| \___| \__,_|(_)
EOF




