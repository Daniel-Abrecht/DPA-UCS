#!/bin/bash

src="$1"
varname="$2"
urlbase="$3"
destbasepath="$4"
destname="$5"
out="$6"

function escape {
  file="$1"
  echo "\"$(echo "$file" | sed 's/\\/\\\\/g' | sed 's/"/\\"/g')\""
}

function createFileIncludeCode {
  folder="$1"
  dest="$2"
  varname="$3"
  baseurl="$4"
  headerfile="$5"
  inc="$6"
  basemacroname="$(echo "FILE_BASE_$varname" | tr '[:lower:]' '[:upper:]')"
  root="$(pwd)"
  pushd "$folder" > /dev/null
  i=0
  echo "
#include \"$headerfile\"
#define $basemacroname $(escape "$baseurl")"
  i=0
  find -type f | sort | while IFS= read file
  do
    i=$(expr $i + 1)
    url="$(echo $file | sed 's/^[^/]\///')"
    einclude="$(escape "$inc/$url.c")"
    echo "const flash char ${varname}_content_$i[] = 
      #include $einclude
    ;"
  done
  echo -n "
const DPAUCS_ressource_file_t $varname[] = {
  "
  find -type f | sort | while IFS= read file
  do
    i=$(expr $i + 1)

    if [ "$(basename "$file")" = "index.html" ];
    then
      efile="$(dirname "$file")/"
    else
      efile="$file"
    fi

    efile="$(escape "$(echo $efile | sed 's/^[^/]\///')")"
    url="$(echo $file | sed 's/^[^/]\///')"
    einclude="$(escape "$inc/$url.c")"
    mkdir -p "$(dirname "$root/$dest/$url.c")"
    cat $file | "$root/2cstr" > "$root/$dest/$url.c"
    mime="$(escape "$(file --mime-type "$file" | sed 's/^.*: //')")"
    hash="$(escape "$(md5sum "$file" | grep -o '^[^ ]*')")"

    echo "{
    {
      &file_ressource_handler.super,
      (const flash char[]){\"/\" $basemacroname $efile},
      sizeof(\"/\" $basemacroname $efile)-1
    },
    (const flash char[]){$mime},
    (const char[]){$hash},
    sizeof(
      ${varname}_content_$i
    )-1,
    ${varname}_content_$i
  }," | head -c -1

  done
  echo -e "
};

const size_t ${varname}_size = sizeof($varname)/sizeof(*$varname);
"
  popd > /dev/null
}

function createFileIncludeHeader {
  varname=$1
  filename=$2
  echo "\
#ifndef ${filename}_G1_H
#define ${filename}_G1_H

#include <DPA/UCS/files.h>

extern const size_t ${varname}_size;
extern const DPAUCS_ressource_file_t $varname[];

#endif
"
}

echo "Generating file: $destbasepath/$out.g1.h"
createFileIncludeHeader "$src" "$out" > "$destbasepath/$out.g1.h"
createFileIncludeCode "$src" "$destbasepath/$destname" "$varname" "$urlbase" "$out.g1.h" "$destname" > "$destbasepath/$out.g1.c"
echo "Done!"
