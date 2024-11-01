//Docker tutorial
//Lab 9
//purpose: Put code on it; deploy at one place


// Build image
// https://devblogs.microsoft.com/cppblog/c-development-with-docker-containers-in-visual-studio-code/
// Step one: Download docker for desktop
// Step Two: create DockerFile (similar to Makefile must capitalize)

# Get the base ubuntu image from Docker Hub (this tutorial is for ubuntu, change this line for different OS. Check Docker Hub web)
FROM ubuntu:latest

# Update apps on the base image
RUN apt-get -y update && apt-get install -y
# Install any library that might need
RUN apt-get install -y git make gcc g++ openssl libssl-dev

# Copy the current folder which contains C++ source code
# to the Docker image under usr/src
# Need to put all the files under docker folder()
COPY . /usr/src/crawler

# Specify the working directory
# Workdir = cd in shell
WORKDIR /usr/src/crawler

# Compile the source files
# If we have make file, we can write make crawler instead
RUN g++ crawler.cpp -o crawler

# Run the program output from the previous step
CMD ["./crawler"]

// Step 3: vscode has a extension for docker; need to install
// Step 4: right click the name of the file, go to build image
// Step 5: give it a name and run -> this will build the image under the name you give
// Step 6: go to a icon looks like a boat, and click run at the name of our image

// Deploy this image to ECS
//https://aws.amazon.com/getting-started/hands-on/deploy-docker-containers/


// Lab 9:
// TODO: send message should have header: what kind of message it is; how big it is

