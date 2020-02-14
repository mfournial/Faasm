ARG FAASM_VERSION
FROM faasm/base:${FAASM_VERSION}

# ------------------------------------------------------
# This image should contain everything needed for tests
# that doesn't need to rebuilt for every run.
# ------------------------------------------------------

RUN apt-get update
RUN apt-get install -y apt-transport-https
RUN apt-get install -y \
    git \
    ssh \
    tar \
    gzip \
    ca-certificates \
    libcairo2-dev \
    python3-dev \
    python3-venv

# Set up Catch
WORKDIR /usr/local/code/faasm/ansible
COPY ansible/catch.yml catch.yml
RUN ansible-playbook catch.yml

# Set up requests
RUN apt-get install -y python3-dev python3-pip
RUN pip3 install invoke requests

# Install other python deps
COPY requirements.txt /tmp/requirements.txt
RUN pip3 install -r /tmp/requirements.txt

# Install pyfaasm
RUN pip3 install pyfaasm

# Download the toolchain
WORKDIR /usr/local/code/faasm
RUN inv download-toolchain
RUN inv download-sysroot
RUN inv download-runtime-root --nocodegen
# Compile test library
RUN inv compile-libfake

# Fix ownership
RUN chown -R root:root /usr/local/faasm

RUN inv run-local-codegen

# Compile test library
RUN inv compile-libfake

# Fix ownership
RUN chown -R root:root /usr/local/faasm

# Create user with dummy uid required by Python
RUN groupadd -g 1000 faasm
RUN useradd -u 1000 -g 1000 faasm

# Build codegen binaries
WORKDIR /faasm/build
RUN cmake --build . --target codegen_shared_obj
RUN cmake --build . --target codegen_func

# Clear out
RUN rm -rf /tmp/*
