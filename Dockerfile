FROM ossrs/srs:dev AS build

COPY . /usr/local/srs-bench
WORKDIR /usr/local/srs-bench
RUN ./configure && make

############################################################
# dist
############################################################
FROM centos:7 AS dist

COPY --from=build /usr/local/bin /usr/local/bin
COPY --from=build /usr/local/srs-bench /usr/local/srs-bench

WORKDIR /usr/local/srs-bench
CMD ["bash", "-c", "ls -lh objs"]

