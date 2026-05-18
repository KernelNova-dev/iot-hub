#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2026 Daryna Vasylchenko (KernelNova) <daryna.vasylchenko@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later


set -euo pipefail

readonly SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
readonly WORKSPACE_HOME="$(cd -- "${SCRIPT_DIR}/.." &>/dev/null && pwd)"
readonly BUILD_DIR="$WORKSPACE_HOME/build"
readonly PACKAGE_DIR="$BUILD_DIR/package"
readonly PACKAGE_ROOT="$PACKAGE_DIR/wiregate"

function usage() {
    cat <<EOF >&2
Usage: $0 ACTION

Actions:
  clean     Remove generated build artifacts.
  build     Install Conan dependencies, configure Meson, and build the project.
  package   Build the project and create a runnable WireGate package.
  rebuild   Clean the build directory and rebuild from scratch.

Examples:
  $0 build
  $0 package
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

activate_conan_build_env() {
    local conan_env="${BUILD_DIR}/conan/conanbuild.sh"

    if [[ ! -f "$conan_env" ]]; then
        echo "Conan build environment not found: $conan_env"
        exit 1
    fi

    # shellcheck disable=SC1090
    source "$conan_env"
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
    activate_conan_build_env

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

copy_built_binaries() {
    local meson_build_dir="${BUILD_DIR}/meson"

    mkdir -p "$PACKAGE_ROOT/bin" "$PACKAGE_ROOT/lib"

    while IFS= read -r binary; do
        local filename
        filename="$(basename "$binary")"

        if [[ "$filename" == *.symbols ]]; then
            continue
        fi

        if [[ "$filename" == *.so || "$filename" == *.so.* ]]; then
            cp -L "$binary" "$PACKAGE_ROOT/lib/$filename"
        else
            cp "$binary" "$PACKAGE_ROOT/bin/$filename"
        fi
    done < <(find "$meson_build_dir/src" -type f \( -perm -111 -o -name "*.so" -o -name "*.so.*" \))
}

create_service_launcher() {
    local launcher_template="$WORKSPACE_HOME/tools/launcher.template"
    local launcher_file="$PACKAGE_ROOT/run-wiregate-service.sh"

    if [[ ! -f "$launcher_template" ]]; then
        echo "Launcher template not found: $launcher_template"
        exit 1
    fi

    cp "$launcher_template" "$launcher_file"
    chmod +x "$launcher_file"
}

create_cli_launcher() {
    local launcher_template="$WORKSPACE_HOME/tools/cli-launcher.template"
    local launcher_file="$PACKAGE_ROOT/cli"

    if [[ ! -f "$launcher_template" ]]; then
        echo "CLI launcher template not found: $launcher_template"
        exit 1
    fi

    cp "$launcher_template" "$launcher_file"
    chmod +x "$launcher_file"
}

create_web_launcher() {
    local launcher_template="$WORKSPACE_HOME/tools/web-launcher.template"
    local launcher_file="$PACKAGE_ROOT/gui"

    if [[ ! -f "$launcher_template" ]]; then
        echo "Web launcher template not found: $launcher_template"
        exit 1
    fi

    cp "$launcher_template" "$launcher_file"
    chmod +x "$launcher_file"
}

copy_web_assets() {
    local web_assets_dir="$WORKSPACE_HOME/src/gui/web"

    if [[ ! -d "$web_assets_dir" ]]; then
        echo "Web assets directory not found: $web_assets_dir"
        exit 1
    fi

    mkdir -p "$PACKAGE_ROOT/web"
    cp -R "$web_assets_dir/." "$PACKAGE_ROOT/web/"
}

package_app() {
    local config_file="$WORKSPACE_HOME/config/wiregate-config.json"
    local web_config_file="$WORKSPACE_HOME/config/wiregate-web.json"
    local license_file="$WORKSPACE_HOME/LICENSE"
    local authors_file="$WORKSPACE_HOME/AUTHORS.md"
    local package_archive="$PACKAGE_DIR/wiregate-$(uname -s | tr '[:upper:]' '[:lower:]')-$(uname -m).tar.gz"

    if [[ ! -f "$config_file" ]]; then
        echo "WireGate config file not found: $config_file"
        exit 1
    fi

    if [[ ! -f "$web_config_file" ]]; then
        echo "WireGate web config file not found: $web_config_file"
        exit 1
    fi

    if [[ ! -f "$license_file" ]]; then
        echo "License file not found: $license_file"
        exit 1
    fi

    if [[ ! -f "$authors_file" ]]; then
        echo "Authors file not found: $authors_file"
        exit 1
    fi

    build_meson

    echo "Creating package directory: $PACKAGE_ROOT"
    rm -rf "$PACKAGE_ROOT"
    mkdir -p "$PACKAGE_ROOT/config"

    copy_built_binaries

    if [[ ! -x "$PACKAGE_ROOT/bin/wiregate-service" ]]; then
        echo "Packaged service executable not found: $PACKAGE_ROOT/bin/wiregate-service"
        exit 1
    fi

    if [[ ! -x "$PACKAGE_ROOT/bin/wiregate-cli" ]]; then
        echo "Packaged CLI executable not found: $PACKAGE_ROOT/bin/wiregate-cli"
        exit 1
    fi

    if [[ ! -x "$PACKAGE_ROOT/bin/wiregate-gui" ]]; then
        echo "Packaged web executable not found: $PACKAGE_ROOT/bin/wiregate-gui"
        exit 1
    fi

    cp "$config_file" "$PACKAGE_ROOT/config/wiregate-config.json"
    cp "$web_config_file" "$PACKAGE_ROOT/config/wiregate-web.json"
    cp "$config_file.license" "$PACKAGE_ROOT/config/wiregate-config.json.license"
    cp "$web_config_file.license" "$PACKAGE_ROOT/config/wiregate-web.json.license"
    cp "$license_file" "$PACKAGE_ROOT/LICENSE"
    cp "$authors_file" "$PACKAGE_ROOT/AUTHORS.md"
    copy_web_assets
    create_service_launcher
    create_cli_launcher
    create_web_launcher

    echo "Creating package archive: $package_archive"
    rm -f "$package_archive"
    tar -C "$PACKAGE_DIR" -czf "$package_archive" wiregate

    echo "Package created: $package_archive"
}

readonly ACTION="${1:-usage}"

case "$ACTION" in
    clean)
        clean
        ;;
    build)
        build_meson
        ;;
    package)
        package_app
        ;;
    rebuild)
        rebuild
        ;;
    *)
        usage
        ;;
esac
