#!/bin/sh

# User Service
systemctl --user stop app-io.github.disresbek.Dili
rm "$HOME/.config/systemd/user/app-io.github.disresbek.Dili.service"
systemctl --user daemon-reload
echo "Sunshine User Service has been removed."

# Remove rules
flatpak-spawn --host pkexec sh -c "rm /etc/modules-load.d/60-sunshine.conf"
flatpak-spawn --host pkexec sh -c "rm /etc/udev/rules.d/60-sunshine.rules"
flatpak-spawn --host pkexec udevadm control --reload-rules
echo "Input rules removed."
