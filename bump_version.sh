#!/usr/bin/env bash

[ -z "$1" ] && {
    echo "Must provide a bump rule"
    exit 1
}

uv version --bump "$1"
sed -i "s/\[Unreleased\]/[$(uv version --short)] $(date +%Y-%m-%d)/" \
    CHANGELOG.md
git add pyproject.toml CHANGELOG.md
git commit -m "Bump version"
git tag "v$(uv version --short)"
