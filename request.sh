#!/bin/sh

curl -v http://localhost:4221/test \
  -H "Host: localhost" \
  -H "User-Agent: curl/7.88.1" \
  -H "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8" \
  -H "Accept-Encoding: gzip, deflate, br" \
  -H "Accept-Language: en-US,en;q=0.5" \
  -H "Connection: keep-alive" \
  -H "Cache-Control: no-cache" \
  -H "Pragma: no-cache" \
  -H "X-Custom-Header: custom-value" \
  -H "Referer: http://localhost:8080/ref" \
  -H "Content-Type: application/json" \
  -H "Authorization: Basic dXNlcjpwYXNz" \
  -H "If-Modified-Since: Wed, 21 Oct 2015 07:28:00 GMT" \
  -d '{"key": "value"}'
