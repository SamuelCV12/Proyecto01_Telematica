FROM python:3.11-slim

WORKDIR /app
COPY protocol_client.py web_service.py ./

ENV OPERATOR_WEB_PORT=3000
EXPOSE 3000

CMD ["python", "web_service.py"]
