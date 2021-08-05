#!/bin/bash
#HELP>#####################################################################
# Runs require.s2 unit test scripts.
#
# Usages:
#
# 1) $0 [flags]
# searches for *.test.s2 in $(dirname $0)/require.d and runs them.
#
# 2) $0 [flags] moduleName ...moduleNameN
# Expects that each argument is a require.s2 module name and
# expects moduleName.test.s2 to exist in the require.s2 module
# search path.
#
# Runs each test in its own s2sh instance and exits with non-0 if
# any test fails.
#
# Flags:
#
# -V enables VERBOSE mode, which simply outputs some extra output.
#
# -vg enables valgrind/massif tests IF valgrind is found in the path.
# These generate some files which one may peruse to collect allocation
# metrics.
#
# Any flags not handled by this script are passed on to the underlying
# s2sh call (quoting might be a problem - avoid complex flags).
#
#HELP<###################################################################

dir=$(dirname $0)
#[[ '.' = "${dir}" ]] && dir=$PWD
s2sh=$dir/f-s2sh

[[ -x "$s2sh" ]] || s2sh=$(which s2sh 2>/dev/null)
[[ -x "$s2sh" ]] || {
    echo "s2sh not found :(. Looked in [${dir}] and \$PATH." 1>&2
    exit 1
}
echo "s2sh = ${s2sh}"
rdir=${REQUIRES2_HOME}
[[ -d "$rdir" ]] || rdir=$dir/require.d
[[ -d "$rdir" ]] || {
    echo "Missing 'required' dir: $rdir" 1>&2
    exit 2
}
rdir=$(cd $rdir>/dev/null; echo $PWD)
export S2_HOME=$PWD # used by require.s2

requireCall="(s2.require ||| import('${rdir}/require.s2'))"

DO_VG=0
VERBOSE=0
S2SH_ADD_FLAGS="${S2SH_XFLAGS}"
DO_DEBUG=0

list=''
for i in "$@"; do
    case $i in
        -V) VERBOSE=1
            ;;
        -vg) DO_VG=1
            ;;
        -debug)
            DO_DEBUG=1
            ;;
        -\?|--help)
            echo "$0:"
            sed -n '/^#HELP/,/^#HELP/p' < $0 | grep -v -e '^#HELP' \
                | sed -e 's/^#/    /g'
            exit 0;
            ;;
        -*)
            S2SH_ADD_FLAGS="${S2SH_ADD_FLAGS} $i"
            ;;
        *) list="$list $i.test"
        ;;        
    esac
done

[[ x = "x${list}" ]] && {
    cd $rdir || exit $? # oh, come on, steve, this isn't C!
    list=$(find . -name '*.test.s2' | cut -c3- | sed -e 's/\.s2$//g' | sort)
    cd - >/dev/null
}

[[ "x" = "x${list}" ]] && {
    echo "Didn't find any *.test.s2 scripts :(" 1>&2
    exit 3
}
list=$(echo $list) # remove newlines
#echo "Unit test list: $list"

function verbose(){
    [[ x1 = "x${VERBOSE}" ]] && echo $@
}

function vgTest(){
    local test=$1
    shift
    local flags="$@"
    local tmp=tmp.$test
    local vgout=vg.$test
    cat <<EOF > $tmp
${requireCall}(['nocache!${test}']);
EOF
    local cmd="$vg --leak-check=full -v --show-reachable=yes --track-origins=yes $s2sh ${S2SHFLAGS} -f ${tmp} ${flags}"
    echo "Valgrind: $test"
    verbose -e "\t$cmd"
    $cmd &> $vgout || {
        rc=$?
        rm -f $tmp
        echo "Valgrind failed. Output is in ${vgout}"
        exit $rc
    }
    #rm -f $vgout
    echo "Valground: $test [==>$vgout]"
    vgout=massif.$test
    local msp=ms_print.$test
    cmd="$massif --massif-out-file=$msp $s2sh ${S2SHFLAGS} -f ${tmp} ${flags}"
    echo "Massifying: $test"
    verbose -e "\t$cmd"
    $cmd &> $vgout || {
        rc=$?
        rm -f $tmp
        echo "Massif failed. Output is in ${vgout}"
        exit $rc
    }
    echo "Massified $test: try: ms_print ${msp} | less"
}

#if [ -e f-s2sh ]; then
#    # kludge for the libfossil source tree
#    S2SH_ADD_FLAGS="${S2SH_ADD_FLAGS}"
#fi
if [[ x1 = x${DO_DEBUG} ]]; then
    s2sh="gdb --args $s2sh"
fi
S2SHFLAGS="-rv -si ${S2SH_ADD_FLAGS}"
# Reminder: some fsl modules rely on code set up by s2sh init script,
# so we cannot use the --a option in libfossil.
echo S2SHFLAGS=$S2SHFLAGS
for test in $list; do
    echo "Running require.s2 test: ${test}"
    outfile=${rdir}/${test}.test_out
    verbose "Output going to: $outfile"
    rm -f "$outfile"
    tmpfile=${rdir}/${test}.tmp
    echo "${requireCall}(['nocache!${test}']);" > $tmpfile
    cmd="$s2sh ${S2SHFLAGS} -o ${outfile} -f ${tmpfile}"
    echo "Running test [${test}]: $cmd"
    $cmd
    rc=$?
    [[ 0 -eq $rc ]] || {
        echo "Test '${test}' failed. Script output (if any) saved to [${outfile}]" 1>&2
        exit $rc
    }
    #echo "Test did not fail: ${test}"
    rm -f $tmpfile
    if [[ -s "$outfile" ]]; then
        verbose -e "\tOutput is in: ${outfile}"
    else
        rm -f "${outfile}"
    fi
done

if [[ x1 = "x$DO_VG" ]]; then
   vg=$(which valgrind)
    if [[ -x "$vg" ]]; then
        echo "Runing test(s) through valgrind..."
        massif="${vg} --tool=massif --time-unit=ms --heap-admin=0"
        for test in $list; do
            outfile=$rdir/${test}.test_out
            rm -f "$outfile"
            vgTest $test -o "${outfile}"
        done
    fi
fi

echo "Done! Tests run: ${list}"
