
z = """
Host: server\r\n
User-Agent: Mozilla/5.0 (X11; Linux x86_64) Gecko/20130501 Firefox/30.0 AppleWebKit/600.00 Chrome/30.0.0000.0 Trident/10.0 Safari/600.00\r\n
Cookie: uid=12345678901234567890; __utma=1.1234567890.1234567890.1234567890.1234567890.12; wd=2560x1600\r\n
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n
Accept-Language: en-US,en;q=0.5\r\n
Connection: keep-alive\r\n\r\n\r\n\r\n\r\n
"""
z = """
https://chattyr.com/sub/p/Jokes/3/hot
authority: chattyr.com
accept: */*
accept-language: en-US,en;q=0.9
cookie: mrsession=sdnNiiH4FUDCYCuwEWrPhWS5zmGZoiOv
dnt: 1
referer: https://chattyr.com/sub/Jokes
sec-ch-ua: ^\^"Not_A Brand^\^";v=^\^"8^\^", ^\^"Chromium^\^";v=^\^"120^\^", ^\^"Google Chrome^\^";v=^\^"120^\^"
sec-ch-ua-mobile: ?0
sec-ch-ua-platform: ^\^"Windows^\^"
sec-fetch-dest: empty
sec-fetch-mode: cors
sec-fetch-site: same-origin
user-agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36
"""

print(len(z))

