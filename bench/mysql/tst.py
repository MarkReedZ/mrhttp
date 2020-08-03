  
import MySQLdb
db=MySQLdb.connect(host="localhost", user="ch", passwd="skiz6oon",db="tech")

c = db.cursor()

for x in range(10000):
  c.execute("insert into World(id,randomNumber) VALUES (%s,%s);",(x,x))
db.commit()
c.close()
db.close()
