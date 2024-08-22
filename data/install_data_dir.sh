#!/bin/sh

if [ -n "$SUDO_USER" ]; then
    USER_HOME=$(eval echo ~$SUDO_USER)
    sudo -u "$SUDO_USER" mkdir -p "$USER_HOME/.local/share/jumpdf"
else
    mkdir -p "$HOME/.local/share/jumpdf"
fi