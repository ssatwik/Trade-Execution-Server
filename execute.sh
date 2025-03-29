#!/bin/bash

inputFile=$1
baseName=$(basename -- "$inputFile")
executableName="${baseName%.*}"

g++ "$inputFile" -o "$executableName"
./"$executableName"
