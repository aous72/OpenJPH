#!/bin/bash
docker run -it -v "$(cd ../.. && pwd)":/src trzeci/emscripten /bin/bash