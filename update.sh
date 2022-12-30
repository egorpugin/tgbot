#!/bin/bash

SW_VERSION=1.1.5
OLD_VERSION=`git tag | tail -n 1`
NEW_VERSION=$1 # Telegram Bot API version (e.g. 6.1, 6.2 etc.)
shift 1
NEW_SW_VERSION=$SW_VERSION.$NEW_VERSION

SED=sed
if [ -x "$(command -v gsed)" ]; then
    SED=gsed
fi

git pull origin master
curl https://core.telegram.org/bots/api > TelegramBotAPI.html
$SED -i -e '$ d' TelegramBotAPI.html # remove the last line with generation time
$SED -i -e "s/$OLD_VERSION/$NEW_VERSION/g" README.md
$SED -i -e "s/$OLD_VERSION/$NEW_SW_VERSION/g" sw.cpp
die() { echo "$*" 1>&2 ; exit 1; }
sw build $* || die "Build failed"
git commit -am "Update Bot API to $NEW_VERSION."
git tag -a $NEW_SW_VERSION -m "$NEW_SW_VERSION"
git push
git push --tags
gh api --method POST /repos/egorpugin/tgbot/releases -f tag_name="$NEW_SW_VERSION" -f name="$NEW_SW_VERSION" -f body="Update to Bot API v$NEW_VERSION." > /dev/null
# why second build? delete?
sw build $* && sw upload org.sw.demo
