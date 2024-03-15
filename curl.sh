for n in {1..10}; do
  curl -d "param1=value1&param2=value2" -X POST http://localhost:8080/form -H "Content-Type: application/x-www-form-urlencoded"
done
