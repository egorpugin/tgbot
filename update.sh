#!/bin/bash

set -x

OLD_VERSION=$1
NEW_VERSION=$2

curl https://core.telegram.org/bots/api > TelegramBotAPI.html
sed -i -e "s/$OLD_VERSION/$NEW_VERSION/g" README.md sw.cpp
git commit -am "Update Bot API to $NEW_VERSION."
git push --all
sw upload org.sw.demo
