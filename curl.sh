
for run in {1..1000}
do
  curl -d "param1=value1&param2=value2" -X POST http://localhost:8080/ -H "Content-Type: application/x-www-form-urlencoded"
done
