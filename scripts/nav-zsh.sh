#!/bin/zsh

NAV_CLIENT="${NAV_CLIENT:-./build/client}"

function _nav_error {
    echo "Error: $1"
    return 1
}

function _nav_usage {
    echo "Usage: nav [command] <arguments>"
    echo ""
    echo "Commands:"
    echo "  [tag]             Navigate to the specified tag."
    echo "  add [tag] [path]  Add a new tag-path association."
    echo "  delete [tag]      Remove the specified tag."
    echo "  show|s            Show all tag-path associations."
    echo "  back|b            Undo the previous action."
    echo "  actions|a         List all recorded actions."
    echo "  reset|ar          Delete all recorded actions."
    return 1
}

function _register_client {
    local connected
    connected=$($NAV_CLIENT $$ register 2>/dev/null)

    if [ -z "$connected" ]; then
        _nav_error "Unable to reach daemon. Is it running?"
        return 1
    fi
}

function _unregister_client {
    $NAV_CLIENT $$ unregister 2> /dev/null > /dev/null
}

# Add cleanup to zshexit hooks array instead of overriding the function
if [[ ! "${zshexit_functions[(r)_unregister_client]}" ]]; then
    zshexit_functions+=(_unregister_client)
fi

function nav {
    # Attempt to register
    _register_client || return 1

    case "$1" in
        add)
            # Command: nav add [tag] [path]
            tag="$2"
            path="$3"
            if [ -z "$tag" ] || [ -z "$path" ]; then
                _nav_usage
            else
                output=$($NAV_CLIENT $$ add "$tag" "$path" 2> /dev/null)
                if [ "$output" = "OK" ]; then
                    echo "Added tag '$tag' with path '$path'"
                else
                    _nav_error "Failed to add tag '$tag'"
                fi
            fi
            ;;
        delete)
            # Command: nav delete [tag]
            tag="$2"
            if [ -z "$tag" ]; then
                _nav_usage
            else
                output=$($NAV_CLIENT $$ delete "$tag" 2> /dev/null)
                if [ "$output" = "OK" ]; then
                    echo "Deleted tag '$tag'"
                else
                    _nav_error "Failed to delete tag '$tag'"
                fi
            fi
            ;;
        show|s)
            # Command: nav show
            output=$($NAV_CLIENT $$ show 2> /dev/null)
            if [ "$output" != "BAD" ]; then
                echo "$output"
            fi
            ;;
        actions|a)
            # Command: nav actions
            output=$($NAV_CLIENT $$ actions 2> /dev/null)
            if [ "$output" != "BAD" ] && [ -n "$output" ]; then
                echo "$output"
            else
                echo "No previous actions found"
            fi
            ;;
        back|b)
            # Command: nav back
            dir=$($NAV_CLIENT $$ pop 2> /dev/null)
            if [ -n "$dir" ] && [ "$dir" != "BAD" ]; then
                cd "$dir" || _nav_error "Failed to navigate to $dir"
            else
                echo "No previous actions found"
            fi
            ;;
        reset|ar)
            output=$($NAV_CLIENT $$ reset 2> /dev/null)
            if [ "$output" = "OK" ]; then
                echo "Action stack cleared"
            fi
            ;;
        *)
            # Command: nav [tag]
            tag="$1"
            if [ -z "$tag" ]; then
                _nav_usage
                return 1;
            fi

            dir=$($NAV_CLIENT $$ get "$tag" 2> /dev/null)

            if [ "$dir" = "BAD" ] || [ -z "$dir" ]; then
                echo "Tag '$tag' not found."
            else
                # Push current directory to the action stack
                output=$($NAV_CLIENT $$ push "$(pwd)" 2> /dev/null)
                if [ "$output" = "OK" ]; then
                    # Change directory to the retrieved path
                    cd "$dir" || _nav_error "Failed to navigate to $dir"
                else
                    _nav_error "Failed to push current directory to action stack"
                fi
            fi
            ;;
    esac
}

# Autocomplete function for the 'nav' command (zsh completion)
function _nav_completion {
    local -a cmd_options tag_options
    local state

    # Define commands
    cmd_options=(show back add delete actions reset)

    # Get tags (without registering, just try to get the list)
    tag_options=(${(f)"$($NAV_CLIENT $$ list 2> /dev/null)"})

    case "$words[2]" in
        delete)
            _describe 'tags' tag_options
            ;;
        add)
            if [[ $CURRENT -eq 3 ]]; then
                # For tag argument, just allow any input
                _message 'tag name'
            elif [[ $CURRENT -eq 4 ]]; then
                # For path argument, use directory completion
                _path_files -/
            fi
            ;;
        *)
            if [[ $CURRENT -eq 2 ]]; then
                local -a all_options
                all_options=($cmd_options $tag_options)
                _describe 'commands and tags' all_options
            fi
            ;;
    esac
}

# Register the autocomplete function for 'nav'
compdef _nav_completion nav
