FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    g++ \
    zstd \
    curl \
    python3 \
    && rm -rf /var/lib/apt/lists/*

RUN curl -fsSL https://ollama.com/install.sh | sh

WORKDIR /app
COPY src/ ./src/
COPY public/ ./public/

RUN mkdir -p bin && \
    g++ -std=c++17 -O2 src/phi_ws_enterprise.cpp \
    -I src -pthread -o bin/divine-bpo

RUN ollama serve & \
    sleep 5 && \
    ollama pull llama3.2

EXPOSE 8092 8093

CMD ["sh", "-c", "ollama serve & sleep 3 && ./bin/divine-bpo"]
