#!/bin/sh

set -eu

#  release_wheels_OSX_arm64.sh
#
# To upload wheels on test.pypi.org:
#   twine upload --repository testpypi dist/*
#
# To upload wheels on pypi.org:
#   twine upload dist/*
#

version=1.0.6

parse_otool_dependencies() {
    awk '/^[[:space:]]/ { sub(/^[[:space:]]+/, ""); sub(/[[:space:]]+\(compatibility version.*$/, ""); print }'
}

fix_macos_library_paths() {
    test -d pyo
    find pyo -type f \( -name '*.dylib' -o -name '*.so' \) -print |
        while IFS= read -r library; do
            library_dir=${library%/*}
            if [ "${library##*.}" = dylib ]; then
                install_name_tool -id "@loader_path/${library##*/}" "$library"
            fi

            dependencies=$(otool -L "$library")
            printf '%s\n' "$dependencies" |
                parse_otool_dependencies |
                while IFS= read -r dependency; do
                    case $dependency in
                        /usr/lib/*|/System/Library/*) continue ;;
                    esac

                    name=${dependency##*/}
                    bundled="$library_dir/$name"
                    if [ ! -f "$bundled" ]; then
                        case $name in
                            *.dylib) ;;
                            *) echo "No bundled library for $dependency in $library" >&2; exit 1 ;;
                        esac
                        set -- "$library_dir/${name%.dylib}".*.dylib
                        if [ "$#" -ne 1 ] || [ ! -f "$1" ]; then
                            echo "Missing or ambiguous bundled library for $dependency in $library" >&2
                            exit 1
                        fi
                        bundled=$1
                    fi

                    relocated="@loader_path/${bundled##*/}"
                    if [ "$dependency" != "$relocated" ]; then
                        install_name_tool -change "$dependency" "$relocated" "$library"
                    fi
                done

            # Do not publish a wheel with unresolved non-system dependencies.
            dependencies=$(otool -L "$library")
            printf '%s\n' "$dependencies" |
                parse_otool_dependencies |
                while IFS= read -r dependency; do
                    case $dependency in
                        /usr/lib/*|/System/Library/*) ;;
                        @loader_path/*)
                            if [ ! -f "$library_dir/${dependency##*/}" ]; then
                                echo "Unresolved dependency $dependency in $library" >&2
                                exit 1
                            fi
                            ;;
                        *) echo "Non-relocatable dependency $dependency in $library" >&2; exit 1 ;;
                    esac
                done
        done
}

sign_macos_libraries() {
    find pyo -type f \( -name '*.dylib' -o -name '*.so' \) -print |
        while IFS= read -r library; do
            codesign --force --sign - "$library"
            codesign --verify "$library"
        done
}

#### Clean up.
rm -rf build dist

for python_version in 3.11 3.12 3.13 3.14; do
    python_tag="cp$(printf '%s' "$python_version" | tr -d '.')"
    /usr/local/bin/python"$python_version" -m build --wheel \
        --config-setting="--build-option=--use-coreaudio" \
        --config-setting="--build-option=--use-double" \
        --config-setting="--build-option=--plat-name=macosx_13_0_arm64"

    wheel_file="pyo-${version}-${python_tag}-${python_tag}-macosx_13_0_arm64.whl"
    dist_info="pyo-${version}.dist-info"

    (
        cd dist
        unzip "$wheel_file"
        fix_macos_library_paths
        sign_macos_libraries
        zip -r -X "$wheel_file" "$dist_info" pyo pyo64
        rm -rf "$dist_info" pyo pyo64
    )
done
