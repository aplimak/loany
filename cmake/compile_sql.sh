#!/usr/bin/env bash
set -euo pipefail

if [ $# -ne 3 ]; then
    printf 'usage: %s <indir> <outfile> <varprefix>\n' "$0" >&2
    exit 1
fi

indir=${1%/} # strip trailing slash so prefix removal is consistent
outfile=$2
varprefix=$3

escape_c() {
    local s=$1
    s=${s//\\/\\\\}
    s=${s//\"/\\\"}
    printf '%s' "$s"
}

{
    # Deterministic order, NUL-safe, handles spaces/newlines in names
    while IFS= read -r -d '' file; do
        rel=${file#"$indir"/} # schema/people.sql  (or people.sql)
        rel=${rel%.sql}       # schema/people      (or people)

        varname=$(printf '%s_%s' "$varprefix" "$rel" |
            tr '[:lower:]' '[:upper:]' |
            tr -c '[:alnum:]_' '_')
        varname=${varname%_} # in case tr left a trailing _ from odd chars

        printf 'static const char %s[] = "' "$varname"
        first=1
        while IFS= read -r line || [ -n "$line" ]; do
            if [ "$first" -eq 0 ]; then
                printf '\\n'
            fi
            first=0
            escape_c "$line"
        done <"$file"
        printf '";\n'
    done < <(find "$indir" -type f -name '*.sql' -print0 | sort -z)
} >"$outfile"
