#!/bin/bash
docker run -it --user $(id -u):$(id -g) -v "$(cd ../.. && pwd)":/src emscripten/emsdk /bin/bash
