#!/usr/bin/env bash

[ -z "$1" ] && {
    echo "Must provide a bump rule"
    exit 1
}

poetry version "$1"
sed -i "s/\[Unreleased\]/[$(poetry version --short)] $(date +%Y-%m-%d)/" \
    CHANGELOG.md
git add pyproject.toml CHANGELOG.md
git commit -m "Bump version"
git tag "v$(poetry version --short)"
