#!/bin/bash

set -e

install_debian() {
    echo "Detected Debian/Ubuntu-based system. Installing dependencies..."
    sudo apt update
    sudo apt install -y build-essential libc6-dev linux-headers-$(uname -r)
}

install_rhel() {
    echo "Detected RHEL/CentOS-based system. Installing dependencies..."
    sudo yum update
    sudo yum install -y gcc make glibc-devel kernel-headers
}

install_arch() {
    echo "Detected Arch Linux-based system. Installing dependencies..."
    sudo pacman -Sy --needed base-devel glibc linux-headers
}

if [ -f /etc/debian_version ]; then
    install_debian
elif [ -f /etc/redhat-release ]; then
    install_rhel
elif [ -f /etc/arch-release ]; then
    install_arch
else
    echo "Unsupported Linux distribution. Please install dependencies manually."
    exit 1
fi

check_dependencies() {
    echo "Verifying installation..."

    if ! gcc --version &>/dev/null; then
        echo "Error: GCC is not installed."
        exit 1
    fi

    if ! make --version &>/dev/null; then
        echo "Error: Make is not installed."
        exit 1
    fi

    echo "All dependencies are installed and verified!"
}

check_dependencies

echo "Installation complete."

echo "Compiling module..."
make
echo "Module compiled successfully."

echo "copy executable to local bin"
sudo cp -v bin/rdgen /usr/local/bin/



