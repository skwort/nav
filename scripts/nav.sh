#!/bin/bash

NAV_CLIENT="${NAV_CLIENT:-./build/client}"

function error {
    echo "Error: $1"
    return 1
}

function nav_usage {
    echo "Usage: nav [command] <arguments>"
    echo ""
    echo "Commands:"
    echo "  [tag]             Navigate to the specified tag."
    echo "  add [tag] [path]  Add a new tag-path association."
    echo "  delete [tag]      Remove the specified tag."
    echo "  show              Show all tag-path associations."
    echo "  back|b            Undo the previous action."
    echo "  actions           List all recorded actions."
    return 1
}

function unregister_client {
    $NAV_CLIENT $$ unregister 2> /dev/null
    echo "Client unregistered"
}

# Unregister the client when shell exits
trap unregister_client EXIT

function nav {
    # Attempt to register
    $NAV_CLIENT $$ register 2> /dev/null > /dev/null

    case "$1" in
        add)
            # Command: nav add [tag] [path]
            tag="$2"
            path="$3"
            if [ -z "$tag" ] || [ -z "$path" ]; then
                nav_usage
            else
                output=$($NAV_CLIENT $$ add "$tag" "$path" 2> /dev/null)
                if [ "$output" == "OK" ]; then
                    echo "Added tag '$tag' with path '$path'"
                else
                    error "Failed to add tag '$tag'"
                fi
            fi
            ;;
        delete)
            # Command: nav delete [tag]
            tag="$2"
            if [ -z "$tag" ]; then
                nav_usage
            else
                output=$($NAV_CLIENT $$ delete "$tag" 2> /dev/null)
                if [ "$output" == "OK" ]; then
                    echo "Deleted tag '$tag'"
                else
                    error "Failed to delete tag '$tag'"
                fi
            fi
            ;;
        show)
            # Command: nav show
            output=$($NAV_CLIENT $$ show 2> /dev/null)
            if [ "$output" != "BAD" ]; then
                echo "$output"
            fi
            ;;
        actions)
            # Command: nav actions 
            output=$($NAV_CLIENT $$ actions 2> /dev/null)
            if [ "$output" != "BAD" ]; then
                echo "$output"
            fi
            ;;
        back|b)
            # Command: nav back
            dir=$($NAV_CLIENT $$ pop 2> /dev/null)
            if [ -n "$dir" ] && [ "$dir" != "BAD" ]; then
                cd "$dir" || error "Failed to navigate to $dir"
            else
                error "No previous directory found in action stack"
            fi
            ;;
        *)
            # Command: nav [tag]
            tag="$1"
            if [ -z "$tag" ]; then
                nav_usage
                return 1;
            fi

            dir=$($NAV_CLIENT $$ get "$tag" 2> /dev/null)

            if [ "$dir" == "BAD" ] || [ -z "$dir" ]; then
                error "Tag '$tag' not found or daemon did not respond"
            else
                # Push current directory to the action stack
                output=$($NAV_CLIENT $$ push "$(pwd)" 2> /dev/null)
                if [ "$output" == "OK" ]; then
                    # Change directory to the retrieved path
                    cd "$dir" || error "Failed to navigate to $dir"
                else
                    error "Failed to push current directory to action stack"
                fi
            fi
            ;;
    esac
}

# Autocomplete function for the 'nav' command
function _nav_autocomplete {
    local cur prev options

    # Current word being completed
    cur="${COMP_WORDS[COMP_CWORD]}"
    # Previous word (last completed)
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    # Define possible commands
    options="show back add delete actions"

    case "$prev" in
        nav)           
            COMPREPLY=( $(compgen -W "$options" -- "$cur") )
            ;;
        add)
            if [[ "$COMP_CWORD" -eq 2 ]]; then
                COMPREPLY=()
            else
                COMPREPLY=( $(compgen -d -- "$cur") )
            fi
            ;;
        *)
            COMPREPLY=()
            ;;
    esac
}

# Register the autocomplete function for 'nav'
complete -F _nav_autocomplete nav
