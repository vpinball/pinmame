FROM ubuntu

RUN apt update && apt install \
    build-essential \
    cmake \
    zlib1g-dev \
    libx11-dev libxv-dev libasound2-dev