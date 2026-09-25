#!/bin/bash
echo "Installing Windows Terminal Aliases (taskmgr, dxdiag, etc)..."
mkdir -p ~/.bashrc.d
cat << 'INNER_EOF' > ~/.bashrc.d/windows_diagnostics.sh
alias taskmgr="plasma-systemmonitor"
alias dxdiag="kinfocenter"
alias eventvwr="ksystemlog"
alias diskmgmt.msc="partitionmanager"
INNER_EOF

echo "Enabling Windows Case-Insensitivity (cd downloads)..."
cat << 'INNER_EOF' > ~/.bashrc.d/case_insensitive.sh
bind 'set completion-ignore-case on'
INNER_EOF

echo "Done! Restart your terminal for aliases to take effect."
