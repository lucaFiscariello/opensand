#!/bin/bash
# Copyright (c) 2023 RomARS
# The code was developed by Mattia Quadrini <quadrini@romars.tech>

# Stop the docker – containers should be removed as well
echo -e "[ ] Container will be stopped"
docker stop opensand-sat
docker stop opensand-st
docker stop opensand-gw
docker stop test-ws-st
docker stop test-ws-gw

# (Redundant) In case containers were not removed (--rm option)
echo -e "\n[ ] Check if removed"
docker rm opensand-sat &> /dev/null
docker rm opensand-st &> /dev/null
docker rm opensand-gw &> /dev/null
docker rm test-ws-st &> /dev/null
docker rm test-ws-gw &> /dev/null
docker ps --all

# Remove unused networks
echo -e "\n[ ] Docker network will be deleted"
docker network prune 

# (Redundant) In case networks were not removed
echo -e "\n[ ] Check if removed"
docker network rm lan_sat &> /dev/null
docker network rm lan_ws_st &> /dev/null
docker network rm lan_ws_gw &> /dev/null
docker network ls
echo "(i) NOTE: configuration folders are still available in the opensand-xxx folders."