#!/usr/bin/env bash

cd website

if [[ "$USE_VENV" == "1" ]]; then
    source env/bin/activate
fi

./manage.py docker_builder
echo Docker builder terminated with code $?.

if [[ "$USE_VENV" == "1" ]]; then
    deactivate
fi

exec $SHELL
