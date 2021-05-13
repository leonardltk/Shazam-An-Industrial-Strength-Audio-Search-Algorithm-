# harbor.bigo.sg/bigo_ai/infer/kafka-consumer:py3.5
FROM python:3.5-alpine
RUN pip install requests 
COPY ./ /app
WORKDIR /app
CMD ["python3", "srv.py"]
