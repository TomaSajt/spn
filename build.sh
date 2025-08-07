#!/usr/bin/env bash
set -e

header() {
	echo -e "\n##### $@\n"
}

header Building GameServer
pushd gameserver
./make.sh -j4
popd

header Building RelayServer
pushd relayserver
./make.sh -j4
popd

header Setting up Django environment and applying migrations
pushd website

if [[ "$USE_VENV" == "1" ]]; then
    if [ ! -e env ]; then
        python3 -m venv env
    fi
    source env/bin/activate
    pip install -r requirements.txt
fi

./manage.py migrate
./manage.py loaddata core/fixtures/ProgrammingLanguage.json

if [[ "$USE_VENV" == "1" ]]; then
    deactivate
fi

popd
