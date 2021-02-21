FROM hydroproject/base:latest

ENV DEMO_HOME /shm-demo

WORKDIR /
# start from project home dir
COPY . $DEMO_HOME
RUN mkdir $DEMO_HOME/build
WORKDIR /$DEMO_HOME/build

RUN cmake .. && make

COPY dockerfiles/start-demo.sh /
CMD bash start-demo.sh
