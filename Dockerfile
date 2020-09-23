FROM debian:jessie
MAINTAINER Daniel Castro
RUN apt-get update -qq
RUN apt-get install -y -qq gcc make
RUN apt-get install -y libpng-dev
RUN apt-get install -y zlib1g-dev

EXPOSE 5000

COPY . ../SO_Container_Service
WORKDIR /SO_Container_Service

RUN gcc -o container_service container_service.c -std=c99 -lpng
CMD ["./container_service"]