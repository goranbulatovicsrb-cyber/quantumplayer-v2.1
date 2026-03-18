#!/bin/bash
echo "Downloading miniaudio.h..."
mkdir -p third_party
curl -L -o third_party/miniaudio.h \
  "https://raw.githubusercontent.com/mackron/miniaudio/master/miniaudio.h"
echo "Done!"
