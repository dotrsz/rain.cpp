# lightweight base image
FROM debian:bullseye-slim

# install build tools and tini
RUN apt-get update && apt-get install -y g++ make tini

# set working directory
WORKDIR /app

# copy source code
COPY . .
COPY themes /app/themes

# build application
RUN make

# use tini as entrypoint to handle signals
ENTRYPOINT ["/usr/bin/tini", "--", "./bin/rain"]
