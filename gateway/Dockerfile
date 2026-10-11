FROM python:3.11-slim

ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1 \
    ECHOEAR_GATEWAY_HOST=0.0.0.0 \
    PORT=8080

WORKDIR /app

COPY requirements.txt ./
RUN pip install --no-cache-dir --requirement requirements.txt \
    && useradd --create-home --uid 10001 echoear

COPY echoear_gateway ./echoear_gateway

USER echoear
EXPOSE 8080

CMD ["python", "-m", "echoear_gateway"]
