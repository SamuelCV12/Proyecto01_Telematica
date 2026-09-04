FROM gcc:13 AS build

WORKDIR /app
RUN apt-get update && apt-get install -y --no-install-recommends libsqlite3-dev && rm -rf /var/lib/apt/lists/*
COPY src/ src/
COPY include/ include/
COPY Makefile .

RUN make all

FROM debian:bookworm-slim

WORKDIR /app
RUN apt-get update && apt-get install -y --no-install-recommends libsqlite3-0 && rm -rf /var/lib/apt/lists/*
COPY --from=build /app/build/servidor .
COPY config/ config/

EXPOSE 9001/udp
EXPOSE 9002/tcp

CMD ["./servidor"]