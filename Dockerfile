# Use Python 3.6 running on Debian Buster
FROM python:3.6-buster

# Install required libraries
RUN apt-get update
RUN apt-get install -y wget gzip libgsl-dev ; apt-get clean

# Copy code into container
WORKDIR /
ADD . ephemeris-compute

# Fetch data
WORKDIR /ephemeris-compute
RUN /ephemeris-compute/setup.sh

