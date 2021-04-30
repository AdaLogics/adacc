#!/bin/bash

set -u

function usage() {
    echo "Usage: $0 -i INPUT_DIR [-o OUTPUT_DIR] TARGET..."
    echo
    echo "Run SymCC-instrumented TARGET in a loop, feeding newly generated inputs back "
    echo "into it. Initial inputs are expected in INPUT_DIR, and new inputs are "
    echo "continuously read from there. If OUTPUT_DIR is specified, a copy of the corpus "
    echo "and of each generated input is preserved there. TARGET may contain the special "
    echo "string \"@@\", which is replaced with the name of the current input file."
    echo
    echo "Note that SymCC never changes the length of the input, so be sure that the "
    echo "initial inputs cover all required input lengths."
}

while getopts "i:a:" opt; do
    case "$opt" in
        i)
            in=$OPTARG
            ;;
        a) 
            adir=$OPTARG
            ;;
        *)
            usage
            exit 1
            ;;
    esac
done

shift $((OPTIND-1))
target=$@
timeout="timeout -k 5 90"

if [[ ! -v in ]]; then
    echo "Please specify the input directory!"
    usage
    exit 1
fi

# Create the work environment
work_dir=$(mktemp -d)
mkdir $work_dir/{next,symcc_out}
touch $work_dir/analyzed_inputs

rm -rf ${adir}
mkdir ${adir}
EXPLORED_PATHS=${adir}/explored_paths.txt
PATH_MODELS=${adir}/path_models.txt
SYMCC_LEGIT_FILES=${adir}/legit-files

touch ${EXPLORED_PATHS}
touch ${PATH_MODELS}
mkdir ${SYMCC_LEGIT_FILES}

out=${adir}/out
mkdir ${out}

function cleanup() {
    rm -rf $work_dir
}

trap cleanup EXIT

# Copy all files in the source directory to the destination directory, renaming
# them according to their hash.
function copy_with_unique_name() {
    local source_dir="$1"
    local dest_dir="$2"

    if [ -n "$(ls -A $source_dir)" ]; then
        local f
        for f in $source_dir/*; do
            local dest="$dest_dir/$(sha256sum $f | cut -d' ' -f1)"
            cp "$f" "$dest"
        done
    fi
}

# Copy files from the source directory into the next generation.
function add_to_next_generation() {
    echo "Copying ${1}"
    local source_dir="$1"
    copy_with_unique_name "$source_dir" "$work_dir/next"
}

# If an output directory is set, copy the files in the source directory there.
function maybe_export() {
    local source_dir="$1"
    if [[ -v out ]]; then
        copy_with_unique_name "$source_dir" "$out"
    fi
}

# Copy those files from the input directory to the next generation that haven't
# been analyzed yet.
function maybe_import() {
    if [ -n "$(ls -A $in)" ]; then
        local f
        for f in $in/*; do
            if grep -q "$(basename $f)" $work_dir/analyzed_inputs; then
                continue
            fi

            if [ -e "$work_dir/next/$(basename $f)" ]; then
                continue
            fi

            echo "Importing $f from the input directory"
            cp "$f" "$work_dir/next"
        done
    fi
}

# Set up the shell environment
export SYMCC_OUTPUT_DIR=$work_dir/symcc_out
export SYMCC_ENABLE_LINEARIZATION=1
# export SYMCC_AFL_COVERAGE_MAP=$work_dir/map


# Run generation after generation until we don't generate new inputs anymore
gen_count=0
while true; do
    # Initialize the generation
    maybe_import
    mv $work_dir/{next,cur}
    mkdir $work_dir/next

    echo "Checking size of symcc output dir"
    ls -la ${SYMCC_OUTPUT_DIR}
    echo "done checking"
    # Run it (or wait if there's nothing to run on)
    # Lets cleanup output directory
    rm -rf ${SYMCC_OUTPUT_DIR} && mkdir ${SYMCC_OUTPUT_DIR}
    if [ -n "$(ls -A $work_dir/cur)" ]; then
        echo "Generation $gen_count..."

        for f in $work_dir/cur/*; do
            echo "Running on $f"
            if [[ "$target " =~ " @@ " ]]; then
                #env SYMCC_EXPLORED_PATHS=explored_paths.txt SYMCC_INPUT_FILE=$f $timeout ${target[@]/@@/$f}  >/dev/null 2>&1
                env SYMCC_LEGIT_FILES=${SYMCC_LEGIT_FILES} SYMCC_PATH_MODELS=${PATH_MODELS} SYMCC_EXPLORED_PATHS=${EXPLORED_PATHS} SYMCC_INPUT_FILE=$f $timeout ${target[@]/@@/$f} 
            else
                $timeout $target <$f >/dev/null 2>&1
            fi

            # Make the new test cases part of the next generation
            add_to_next_generation $work_dir/symcc_out
            maybe_export $work_dir/symcc_out
            echo $(basename $f) >> $work_dir/analyzed_inputs
            rm -f $f
        done

        rm -rf $work_dir/cur
        gen_count=$((gen_count+1))
    else
        echo "No more inputs, breaking"
        break
        #rmdir $work_dir/cur
        #sleep 5
    fi

    echo "--- iteration" >> ${adir}/tmps-1.txt
    #cat ./corpus_counters.stats | grep "0" | wc -l >> ${adir}/tmps-1.txt
    echo ">>> Generation ${gen_count}" >> ${adir}/tmps-1.txt
    cat ./corpus_counters.stats >> ${adir}/tmps-1.txt
    cp corpus_counters.stats ${adir}/generation-${gen_count}.stats
    ls -la ${out} | wc -l >> ${adir}/tmps-1.txt
    #echo "Next dir: ${work_dir}/next"
    #ls -la ${work_dir}/next
    #sleep 4
done

echo "Completed execution"
total_files=$(ls -l ${out} | wc -l)
echo "Total inputs found: ${total_files}"

total_paths=$(ls -l ${SYMCC_LEGIT_FILES} | wc -l)
echo "Total unique paths executed: ${total_paths}"
