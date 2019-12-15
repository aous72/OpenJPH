#!/bin/bash
docker run -it --user $(id -u):$(id -g) -v "$(cd ../.. && pwd)":/src trzeci/emscripten /bin/bash