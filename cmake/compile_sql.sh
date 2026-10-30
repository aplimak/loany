#!/usr/bin/env bash
set -euo pipefail

if [ $# -ne 3 ]; then
    printf 'usage: %s <infile> <outfile> <varname>\n' "$0" >&2
    exit 1
fi

readonly infile=$1
readonly outfile=$2
readonly varname=$(printf '%s' "$3" | tr '[:lower:]' '[:upper:]')

# Escape only the two chars C cares about inside "..." literals.
escape_c() {
    local s=$1
    s=${s//\\/\\\\} # \  -> \\
    s=${s//\"/\\\"} # "  -> \"
    printf '%s' "$s"
}

{
    printf 'static const char %s[] = "' "$varname"
    first=1
    while IFS= read -r line || [ -n "$line" ]; do
        if [ "$first" -eq 0 ]; then
            printf '\\n' # emits the two chars  \  n
        fi
        first=0
        escape_c "$line"
    done <"$infile"
    printf '";\n'
} >"$outfile"
