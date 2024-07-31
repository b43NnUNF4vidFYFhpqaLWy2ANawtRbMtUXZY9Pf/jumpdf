#!/bin/sh

if [ -n "$SUDO_USER" ]; then
    USER_HOME=$(eval echo ~$SUDO_USER)
else
    USER_HOME=$HOME
fi

CONFIG_DIR="$USER_HOME/.config/jumpdf"
CONFIG_FILE="$CONFIG_DIR/config.toml"
SOURCE_FILE="$(dirname "$0")/config.toml"

mkdir -p "$CONFIG_DIR"

if [ ! -f "$CONFIG_FILE" ]; then
    cp "$SOURCE_FILE" "$CONFIG_FILE"
fi

if [ -n "$SUDO_USER" ]; then
    chown -R "$SUDO_USER:$SUDO_USER" "$CONFIG_DIR"
fi