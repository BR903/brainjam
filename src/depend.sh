#!/bin/sh
#
# Usage: depend.sh SRCFILE DEPFILE [CC [CFLAGS]]
#
# Output a dependency file for the given source file that includes the
# dependency file itself as a target. Additionally, look for uses of
# the INCBIN macro, and add any data files included into the source as
# dependencies. (Since this latter task is being done with a regular
# expression instead of by parsing the actual C, it is necessarily
# imperfect and depends upon the code not having e.g. an occurrence of
# "INCBIN" inside of a string constant.)

src="$1"
dir=$(dirname "$src")
dep="$2"
if test $# -gt 2 ; then
  CC="$3"
  shift 3
else
  CC="${CC-gcc}"
  shift 2
fi

trap "rm -f ${dep}~" EXIT
printf '%s %s/' "$dep" "$dir" >"${dep}~"
"$CC" -MM -MG "$@" "$src" | sed '$s/$/ \\/' >>"${dep}~"
incbins=$(sed -n 's/^.*INCBIN[ \t]*([ \t]*\"\([^\"]*\)\".*$/\1/p' "$src")
echo $incbins >>"${dep}~"
mv -f "${dep}~" "$dep"

