#!/bin/bash
# Ensures a directory exists and is writable by the current user.
# Uses sudo if necessary (requires passwordless sudo for mkdir/chown).

DIR="$1"

if [ ! -d "$DIR" ]; then
  if ! mkdir -p "$DIR" 2>/dev/null; then
    sudo mkdir -p "$DIR"
    sudo chown $(id -u):$(id -g) "$DIR"
  fi
elif [ ! -w "$DIR" ]; then
  sudo chown $(id -u):$(id -g) "$DIR"
fi
