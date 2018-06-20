#!/bin/bash

CYAN='\033[0;36m'
NC='\033[0m' # No Color

if [ -e install_manifest.txt ]
then
    echo -e "${CYAN}Uninstall the project...${NC}"
    xargs rm -v < install_manifest.txt
    rm -f install_manifest.txt
fi
