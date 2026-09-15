FROM python:3.12-slim

WORKDIR /app

COPY nodes/ /app/nodes/

ENV PYTHONUNBUFFERED=1

CMD ["sh", "-c", "python -m nodes.launcher --count ${NODE_COUNT:-5} --non-interactive"]
