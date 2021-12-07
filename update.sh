#!/bin/bash

SW_VERSION=1.1.4
OLD_VERSION=$1
NEW_VERSION=$2

curl https://core.telegram.org/bots/api > TelegramBotAPI.html
sed -i '$ d' TelegramBotAPI.html # remove the last line with generation time
sed -i -e "s/$OLD_VERSION/$NEW_VERSION/g" README.md sw.cpp
git commit -am "Update Bot API to $NEW_VERSION."
git tag -a $SW_VERSION.$NEW_VERSION -m "$SW_VERSION.$NEW_VERSION"
git push --tags
sw build && sw upload org.sw.demo
