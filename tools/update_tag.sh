#!/bin/sh

git tag -d $1
git push origin :refs/tags/$1
git tag $1 $2
git push origin $1