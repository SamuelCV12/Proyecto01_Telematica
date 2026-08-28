FROM gcc:13 AS build

WORKDIR /app
COPY src/ src/
COPY include/ include/
COPY Makefile .

RUN make all

FROM debian:bookworm-slim

WORKDIR /app
COPY --from=build /app/build/servidor .
COPY config/ config/

EXPOSE 9001/udp
EXPOSE 9002/tcp

CMD ["./servidor"]