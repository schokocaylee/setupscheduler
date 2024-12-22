FROM gcc:14 AS scheduler-build
RUN apt update && apt install -y --no-install-recommends cmake libcjson-dev
COPY setupscheduler /usr/local/src/setupscheduler
WORKDIR /usr/local/src/setupscheduler
RUN ./autogen.sh
RUN make

FROM node:22 AS web-build
COPY scheduler-web /usr/local/src/scheduler-web
WORKDIR /usr/local/src/scheduler-web
RUN npm ci
RUN npm run build

FROM node:22
RUN apt update && apt install -y --no-install-recommends libcjson1
WORKDIR /opt/scheduler-web
COPY --from=scheduler-build /usr/local/src/setupscheduler/setupscheduler /usr/local/bin
COPY --from=web-build /usr/local/src/scheduler-web/.output .
ENV NUXT_SCHEDULER_COMMAND=/usr/local/bin/setupscheduler
RUN mkdir -p /srv/scheduler-data
ENV NUXT_DATA_PATH=/srv/scheduler-data
CMD ["node", "server/index.mjs"]
