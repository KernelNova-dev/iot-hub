#!/usr/bin/env bash

set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
readonly WORKSPACE_HOME="$(cd -- "${SCRIPT_DIR}/.." &>/dev/null && pwd)"
readonly BUILD_DIR="$WORKSPACE_HOME/build"

function usage() {
    cat <<EOF >&2
Usage: $0 ACTION

Actions:
  clean     Remove generated build artifacts.
  build     Install Conan dependencies, configure Meson, and build the project.
  rebuild   Clean the build directory and rebuild from scratch.

Examples:
  $0 build
  $0 rebuild
EOF
    exit 1
}

prepare_conan_profile() {
    local __out_var="${1:-}"
    if [[ -z "$__out_var" ]]; then
        echo "prepare_conan_profile requires output variable name"
        exit 1
    fi

    local template_profile="$WORKSPACE_HOME/tools/profiles/profile.default"
    local resolved_profile="$BUILD_DIR/conan/profile.default"

    if [[ ! -f "$template_profile" ]]; then
        echo "Conan profile template not found: $template_profile"
        exit 1
    fi

    mkdir -p "${BUILD_DIR}/conan" "$HOME/.conan2/profiles"

    cp "$template_profile" "$resolved_profile"
    cp "$resolved_profile" "$HOME/.conan2/profiles/default"

    echo "Conan default profile synced from tools/profiles/profile.default"
    printf -v "$__out_var" '%s' "$resolved_profile"
}

conan_install_deps() {
    mkdir -p "${BUILD_DIR}/conan"

    local profile_file=""
    prepare_conan_profile profile_file

    echo "Installing Conan dependencies"
    conan install "${WORKSPACE_HOME}" \
        --profile:host="$profile_file" \
        --profile:build="$profile_file" \
        --output-folder="${BUILD_DIR}/conan" \
        --build=missing
}

configure_meson() {

    local meson_build_dir="${BUILD_DIR}/meson"
    local conan_native="${BUILD_DIR}/conan/conan_meson_native.ini"

    # Conan may generate Meson native file in nested generator dirs (e.g. build-release/conan).
    if [[ ! -f "$conan_native" ]]; then
        local discovered_native=""
        discovered_native="$(find "${BUILD_DIR}/conan" -type f -name conan_meson_native.ini 2>/dev/null | head -n1 || true)"
        if [[ -n "$discovered_native" ]]; then
            conan_native="$discovered_native"
        fi
    fi

    echo "Configuring Meson build directory: $meson_build_dir"

    if [[ -f "$conan_native" ]]; then
        if [[ -d "$meson_build_dir" ]]; then
            meson setup "$meson_build_dir" "${WORKSPACE_HOME}" --native-file "$conan_native" --reconfigure
        else
            meson setup "$meson_build_dir" "${WORKSPACE_HOME}" --native-file "$conan_native"
        fi
    else
        echo "Conan native file not found, configuring Meson without Conan native profile"
        if [[ -d "$meson_build_dir" ]]; then
            meson setup "$meson_build_dir" "${WORKSPACE_HOME}" --reconfigure
        else
            meson setup "$meson_build_dir" "${WORKSPACE_HOME}"
        fi
    fi
}

build_meson() {
    local meson_build_dir="${BUILD_DIR}/meson"
    
    echo "Ensuring Conan dependencies are installed"
    conan_install_deps

    echo "Ensuring Meson build directory is configured"
    configure_meson
    
    if [[ ! -f "$meson_build_dir/build.ninja" ]]; then
        echo "Meson build directory is not configured correctly: $meson_build_dir"
        exit 1
    fi
    echo "Building project"
    meson compile -C "$meson_build_dir"
}

function clean() {
    echo "Cleaning..."
    rm -rf "$BUILD_DIR"
}

function rebuild() {
    clean
    build_meson
}

readonly ACTION="${1:-usage}"

case "$ACTION" in
    clean)
        clean
        ;;
    build)
        build_meson
        ;;
    rebuild)
        rebuild
        ;;
    *)
        usage
        ;;
esac