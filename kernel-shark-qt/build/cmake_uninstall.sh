#!/bin/bash

CYAN='\033[0;36m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

if [[ $EUID -ne 0 ]]; then
   echo -e "${PURPLE}Permission denied${NC}" 1>&2
   exit 100
fi

if [ -e install_manifest.txt ]
then
    echo -e "${CYAN}Uninstall the project...${NC}"
    xargs rm -v < install_manifest.txt
    rm -f install_manifest.txt
fi
