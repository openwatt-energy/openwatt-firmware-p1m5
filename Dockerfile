# OpenWatt P1M5 firmware build environment.
FROM python:3.11-slim

LABEL org.opencontainers.image.title="openwatt-firmware-p1m5"
LABEL org.opencontainers.image.description="PlatformIO build environment for the OpenWatt P1M5 firmware"

ENV PIP_DISABLE_PIP_VERSION_CHECK=1

# git is required by PlatformIO to fetch platforms and libraries.
RUN apt-get update \
    && apt-get install -y --no-install-recommends git ca-certificates \
    && rm -rf /var/lib/apt/lists/*

RUN pip install --no-cache-dir "platformio==6.1.19"

WORKDIR /workspace

# Warm the ESP32 platform + toolchain + libraries into the image so
# subsequent builds don't re-download them.
COPY platformio.ini ./
RUN pio pkg install

CMD ["pio", "run"]
