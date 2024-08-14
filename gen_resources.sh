#!/usr/bin/env bash

linker="x86_64-w64-mingw32-ld"

# Select output types
for i in {0..1}
do
    case "$1" in
        --header)
            gen_header=1
            shift
            ;;
        --object)
            gen_object=1
            shift
            ;;
    esac 
done

# Read project directory
proj_dir=`realpath "$1"`
shift

# Read output file destinations and make sure they don't exist
if [ "x${gen_object}" = "x1" ]; then
    resources_o=`realpath "$1"`
    shift

    rm -f "${resources_h}"
fi
if [ "x${gen_header}" = "x1" ]; then
    resources_h=`realpath "$1"`
    shift

    rm -f "${resources_o}"
fi

# Recomupte relative paths to parameters
idx=0
resource_files=()

for path in "$@"
do
    resource_files["${idx}"]=`realpath --relative-to="${proj_dir}" "${path}"`
    idx="$(("${idx}" + 1))"
done

if [ "x${gen_object}" = "x1" ]; then
    # Create the object file
    pushd "${proj_dir}" >> /dev/null
    $linker -r -b binary -o "${resources_o}" "${resource_files[@]}"
    popd >> /dev/null
fi

if [ "x${gen_header}" = "x1" ]; then
    # Include stddef.h in the resources header (for size_t)
    echo "#include <stddef.h>" >> "${resources_h}"

    for resource in "${resource_files[@]}"
    do
        # Use relative path to the resource as the variable name
        var_name="_binary_${resource}"

        # Replace all non-alphanumeric characters with underscores
        var_name=`printf "${var_name}" | sed "s/[^a-zA-Z0-9]/_/g"`

        # Define externs in the header
        echo "extern void *${var_name}_start;" >> "${resources_h}"
        echo "extern void *${var_name}_size;" >> "${resources_h}"
        echo "" >> "${resources_h}"
    done
fi
