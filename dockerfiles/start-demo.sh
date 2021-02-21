#!/bin/bash

cd $DEMO_HOME/

if [[ "$ROLE" == 's' ]]; then
  # run server
  ./build/bin/kvs s
else
  # run client
  sleep 3600
fi
