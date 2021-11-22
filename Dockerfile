# Use Python 3.10 running on Debian Bullseye
FROM python:3.10-bullseye

# Install required libraries
RUN apt-get update
RUN apt-get install -y wget gzip libgsl-dev ; apt-get clean

# Copy code into container
WORKDIR /
ADD . ephemeris-compute

# Fetch data
WORKDIR /ephemeris-compute
RUN /ephemeris-compute/setup.sh

# Check that binary quick-lookup files are generated
RUN ./bin/ephem.bin

