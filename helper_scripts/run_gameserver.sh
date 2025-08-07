#!/usr/bin/env bash

cd gameserver
build/GameServer

echo GameServer terminated with code $?.
exec $SHELL
