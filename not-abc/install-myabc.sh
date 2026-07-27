#!/usr/bin/env bash

set -e

install_dir="$HOME/.local/bin"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

add_to_path()
{
    local profile_file=$1
    local path_line='export PATH="$HOME/.local/bin:$PATH"'

    if [[ -f "$profile_file" ]] && grep -Fqx "$path_line" "$profile_file"; then
        return
    fi

    {
        echo
        echo "# Added by install-myabc.sh"
        echo "$path_line"
    } >> "$profile_file"

    profile_changed=true
    profile_name=$profile_file
}

install_program()
{
    local source="$1"
    local target=$2

    if [[ ! -f "$source" ]]; then
        echo "Warning: '$source' not found. Skipping."
        return
    fi

    cp "$source" "$target"
    chmod +x "$target"
}

echo "Installing myabc tools..."

mkdir -p "$install_dir"

profile_changed=false
profile_name=

case ":$PATH:" in
    *":$install_dir:"*)
        ;;
    *)
        shell_name=$(basename "${SHELL:-}")
	echo "shell is ${shellname}"

        case "$shell_name" in
            zsh)
                add_to_path "$HOME/.zprofile"
                ;;
            bash)
                add_to_path "$HOME/.bash_profile"
                ;;
            *)
                echo "Warning: Could not determine whether Bash or Zsh is used." >&2
                echo "Please add the following line to your shell profile:" >&2
                echo '    export PATH="$HOME/.local/bin:$PATH"' >&2
                ;;
        esac
        ;;
esac

install_program \
    "$script_dir/not-abc-wrapper/not-abc_simple" \
    "$install_dir/not-abc_simple"

install_program \
    "$script_dir/not-abc-wrapper/not-abc_llvm" \
    "$install_dir/not-abc_llvm"

install_program \
    "$script_dir/simple/ulmas" \
    "$install_dir/ulmas_simple"

install_program \
    "$script_dir/simple/ulm" \
    "$install_dir/ulm_simple"

install_program \
    "$script_dir/simple/udb-tui" \
    "$install_dir/udb-tui_simple"

for xtest_abc in $script_dir/not-abc-written-in-abc_*; do
    install_program \
	${xtest_abc} \
	"$install_dir/"
done


echo
echo "Installation completed."
echo "The tools were installed in:"
echo "    $install_dir"

if $profile_changed; then
    echo
    echo "The PATH was added to:"
    echo "    $profile_name"
    echo
    echo "Restart the terminal or run:"
    echo "    source \"$profile_name\""
fi
