
import tests.common
import requests
import mrpacker
from tests.common import eq,contains,stop_server

#process.terminate()

longtext = """
Once upon a time in a far-off kingdom, there was a small village known for its peculiar tradition. Every year, they held a competition to find the best story-teller in the village. This year, the competition was fierce, but the most anticipated contender was a man named Jack, famous for his long and humorous stories.

The day of the competition arrived, and the village square was packed with people eager to hear the stories. One by one, the contenders took the stage, each telling a more elaborate tale than the last. Finally, it was Jack's turn.

Jack cleared his throat and began, "In this very village, many years ago, there lived an old farmer named Tom. Tom had a chicken, but not just any chicken. This chicken laid golden eggs! Yes, golden eggs! Every morning, Tom would find a new golden egg in the chicken's nest. He became the richest man in the village. But Tom was greedy. He thought, 'If the chicken can lay golden eggs, its insides must be made of gold!'"

The crowd gasped. Jack continued, "So, one day, Tom decided to cut open the chicken to get all the gold. But when he did, he found out the chicken was just like any other chicken on the inside. Tom was horrified. He realized his greed had cost him his precious source of gold."

The villagers nodded, recognizing the moral of the story. But Jack wasn't finished. "Now, you must be thinking this is the end of the story," he said, "but remember, I'm known for long stories."

He went on, "The spirit of the chicken was so moved by Tom's sorrow and regret that it decided to give him another chance. It appeared to Tom in a dream and said, 'I will give you a quest. If you succeed, you will find something more precious than gold.' The next morning, Tom woke up to find a map left by the chicken's spirit. The map led to a distant mountain where a rare, golden apple grew at the very top. This apple would grant wisdom to whoever ate it."

The villagers leaned in, hooked by the tale. "Tom embarked on a long and perilous journey. He fought off bandits, escaped wild beasts, and climbed the tallest mountains. Finally, after many hardships, he reached the top and found the golden apple."

Everyone waited with bated breath. "Tom picked the apple, and as soon as he took a bite, he was filled with the wisdom of the ages. He understood everything - why the sky is blue, why the birds sing, but most importantly, he understood the folly of greed."

Jack paused, looking around at the captivated audience. "Tom returned to the village a changed man. He shared his newfound wisdom with everyone and lived the rest of his days in peace and happiness. And the moral of the story? It's not the gold or riches that matter, but the wisdom and experiences we gain in our pursuit of them."

The crowd erupted in applause, impressed by the depth and length of Jack's story. As they clapped, Jack smiled, but then he said, "But wait, the story isn't over yet!"

The crowd groaned. Jack chuckled and said, "Just kidding! That's the end."

And that's how Jack won the competition, not just with his long story, but with his clever twist at the end. The villagers learned a valuable lesson about patience and humor, and they talked about Jack's legendary story for many years to come.
"""

server = None
def setup():
  print("Begin test_requests")
  global server
  server = tests.common.start_server("tests/s1.py")  
  if not server:
    return 1
  return 0


def test_one():
  data = {}
  s = "lo(ng"*5000
  data["long"] = s
  r = requests.post('http://localhost:8080/form',data) # TODO have this timeout quickly
  eq(r.status_code, 200)
  eq(r.text, '{"long":"' + s + '"}')

  r = requests.get('http://localhost:8080/foo')
  eq(r.status_code, 200)
  r = requests.get('http://localhost:8080/to64')
  eq(r.text, "ok")
  r = requests.get('http://localhost:8080/foo/baz/whatever')
  eq(r.text, "foo2")
  r = requests.get('http://localhost:8080/foo/bar/whatever')
  eq(r.text, "foo3")
  r = requests.get('http://localhost:8080/foo/baz/w')
  eq(r.text, "foo2")
  r = requests.get('http://localhost:8080/foo/bar/w')
  eq(r.text, "foo3")
  r = requests.get('http://localhost:8080/food/bar/w')
  eq(r.text, "barmid")
  r = requests.get('http://localhost:8080/food/bar/whhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh/')
  eq(r.text, "barmid")
  r = requests.get('http://localhost:8080/print/caf%C3%A9/')
  eq(r.text, "café")
  r = requests.get('http://localhost:8080/print/%E4%B8%8D%E5%8F%AF%E5%86%8D%E7%94%9F%E8%B5%84%E6%BA%90/?test')
  eq(r.text, "不可再生资源")

  # Multiple args
  r = requests.get('http://localhost:8080/args/a/b/c/d/e/f/')
  eq(r.text, "abcdef")
  r = requests.get('http://localhost:8080/args/5/')
  eq(r.text, "5")
  r = requests.get('http://localhost:8080/args/9999999999/')
  eq(r.text, "9999999999")

  # Query string ?foo=bar
  r = requests.get('http://localhost:8080/query_string')
  eq(r.text, "{}")
  r = requests.get('http://localhost:8080/query_string?foo=bar')
  eq(r.text, "{'foo': 'bar'}")
  r = requests.get('http://localhost:8080/query_string?a=b&foo=bar&ABCDE=01234567890123456789')
  eq(r.text, "{'a': 'b', 'foo': 'bar', 'ABCDE': '01234567890123456789'}")

  # requests orders cookie keys alphabetically so expect that
  cookie = {'foo': 'bar'}
  r = requests.post('http://localhost:8080/printCookies', cookies=cookie)
  eq(r.text, "{'foo': 'bar'}")
  cookie = {'foo': 'bar','yay':'test'}
  r = requests.post('http://localhost:8080/printCookies', cookies=cookie)
  eq(r.text, "{'foo': 'bar', 'yay': 'test'}")
  cookie = {'foo': 'bar','baz':'3'}
  r = requests.post('http://localhost:8080/printCookies', cookies=cookie)
  eq(r.text, "{'baz': '3', 'foo': 'bar'}")

  cookie = {'foo': 'b=ar'}
  r = requests.post('http://localhost:8080/printCookies', cookies=cookie)
  eq(r.text, "{'foo': 'b=ar'}")

  # json and mrpacker
  r = requests.post('http://localhost:8080/printPostBody', json={"key": "value"})
  eq(r.text, '{"key": "value"}')
  r = requests.post('http://localhost:8080/json', json={"name": "value"})
  eq(r.text, 'value')
  headers = {'Content-type': 'application/mrpacker'}
  o = { "typ":"post", "s":2, "t": 'Blonde: "What does IDK stand for?"', "l":"localhost/sub/3", "txt": 'Brunette: "I don’t know."\nBlonde: "OMG, nobody does!"' }
  r = requests.post('http://localhost:8080/mrp', data=mrpacker.pack(o), headers=headers)
  eq(r.text, 'post')
  o = { "typ":"post", "s":2, "t": longtext, "l":"localhost/sub/3", "txt": 'Brunette: "I don’t know."\nBlonde: "OMG, nobody does!"' }
  r = requests.post('http://localhost:8080/mrp', data=mrpacker.pack(o), headers=headers)
  eq(r.text, 'post')


  #cookie = {'foo': 'b ar', 'zoo':'animals'}
  #r = requests.post('http://localhost:8080/printCookies', cookies=cookie)
  #eq(r.text, "{'zoo': 'animals'}")
  #cookie = {'foo': 'b\\ar', 'zoo':'animals'}
  #r = requests.post('http://localhost:8080/printCookies', cookies=cookie)
  #eq(r.text, "{'zoo': 'animals'}")

  # Form handling
  r = requests.post('http://localhost:8080/form', data={"p1":"v1","param2":"value2"})
  eq(r.text, '{"p1":"v1","param2":"value2"}')
  r = requests.post('http://localhost:8080/form', data={"":"v","pa{}ram2":"val(ue2"})
  eq(r.text, '{"":"v","pa{}ram2":"val(ue2"}')
  r = requests.post('http://localhost:8080/form', data={"":"v","英文版本":"val(ue2"})
  eq(r.text, '{"":"v","英文版本":"val(ue2"}')
  r = requests.post('http://localhost:8080/form', data={"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa":"","英":"+=&ue2"})
  eq(r.text, '{"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa":"","英":"+=&ue2"}')
  #data = {}
  #s = "lo(ng"*10000
  #data["long"] = s
  #r = requests.post('http://localhost:8080/form',data)
  #eq(r.status_code, 200)
  #eq(r.text, '{"long":"' + s + '"}')
  r = requests.get('http://localhost:8080/form')
  eq(r.text, "No form")

  # Sessions
  cookie = {'mrsession': '43709dd361cc443e976b05714581a7fb'}
  r = requests.post('http://localhost:8080/s', cookies=cookie)
  eq(r.text, "session")

  # Misc
  r = requests.get('http://localhost:8080/printIP')
  eq(r.text, "None")

  #contains(r.text, "Internal Server Error")
  #print( r.status_code, r.headers, r.text )

def teardown():
  global server
  stop_server(server)
  #server.terminate()

