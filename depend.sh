#!/bin/sh
#
# Usage: depend.sh SRCFILE DEPFILE [CFLAGS]
#
# Output a dependency file for the given source file that includes the
# dependency file itself as a target.
#
# gcc doesn't recognize the .incbin assembler directive as creating a
# dependency, so when the source file is of type .S, the script has to
# find these itself. Since binary data files can't themselves be
# dependent on other files, this approach should be effective.

src="$1"
dir=$(dirname "$src")
dep="$2"
shift 2

printf '%s %s/' "$dep" "$dir" >"$dep"
case "$src" in
  *.S)
    gcc -MM -MG "$@" "$src" | head -c-1 >>"$dep"
    echo ' \\' >>"$dep"
    echo $(sed -n 's/^ *\.incbin *\"\(.*\)\".*/\1/p' "$src") >>"$dep"
    ;;
  *)
    gcc -MM -MG "$@" "$src" >>"$dep"
    ;;
esac
