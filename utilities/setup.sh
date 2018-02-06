#!/bin/bash

make -C BaseTools
. edksetup.sh
make -C BaseTools
export EDK_TOOLS_PATH=$HOME/edk2/src/edk2/BaseTools
. edksetup.sh BaseTools

