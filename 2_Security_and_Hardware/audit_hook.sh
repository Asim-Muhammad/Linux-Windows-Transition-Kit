#!/bin/bash
# ==============================================================================
# Bash Audit Hook
# Logs commands to the encryption daemon. Ignores commands starting with a space
# to prevent secrets (API keys/passwords) from being logged.
# ==============================================================================

# Prevent commands starting with a space from entering history
export HISTCONTROL=ignorespace:ignoredups

# Hook command execution
export PROMPT_COMMAND='RETRN_VAL=$?; logger -p local6.debug "cmd: $(history 1 | sed "s/^[ ]*[0-9]\+[ ]*//")";'
