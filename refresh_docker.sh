#!/bin/bash

docker system prune -a
docker container ls | grep test | tr -s ' ' | cut -d ' ' -f 1 | xargs docker container rm
docker volume ls | grep local | tr -s ' ' | cut -d ' ' -f 2 | xargs docker volume rm
docker image ls | grep test | tr -s ' ' | cut -d ' ' -f 3 | xargs docker image rm
docker system prune -a
