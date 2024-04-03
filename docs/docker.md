# Compiling and Running in Docker #

## Step 1 - clone repository   
`https://github.com/aous72/OpenJPH.git`

## Step 2 - build docker image  
`cd OpenJPH`   
`docker build --rm -f Dockerfile -t openjph:latest .`

## Step 3 - run docker image

### in isolated container   
`docker run -it --rm openjph:latest`

### mapping /usr/src/openjph/build directory in the container to local windows c:\temp
`docker run -it --rm -v C:\\temp:/usr/src/openjph/build openjph:latest`
