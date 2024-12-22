# setupscheduler

```
docker build -t setupscheduler:dev .
docker run -v ./scheduler-web/data:/srv/scheduler-data -p 3000:3000 setupscheduler:dev
```
