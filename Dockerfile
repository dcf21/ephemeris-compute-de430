# Use Python 3.12 running on Ubuntu 24.04
FROM ubuntu:24.04

# Install required libraries
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get install -y apt-utils dialog file git vim python3 python3-dev \
                       build-essential make gcc wget gzip libgsl-dev \
                       pkg-config \
                       ; apt-get clean

# Copy code into container
WORKDIR /
ADD . ephemeris-compute

# Fetch data
WORKDIR /ephemeris-compute
RUN /ephemeris-compute/prettymake clean
RUN /ephemeris-compute/setup.sh

# Check that binary quick-lookup files are generated
RUN ./bin/ephem.bin
