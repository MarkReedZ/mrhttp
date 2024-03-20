-- example script demonstrating HTTP pipelining

init = function(args)
  local r = {}
  wrk.headers["Content-Type"] = "application/x-www-form-urlencoded"
  wrk.headers["User-Agent"] = "Mozilla/5.0 (X11; Linux x86_64) Gecko/20130501 Firefox/30.0 AppleWebKit/600.00 Chrome/30.0.0000.0 Trident/10.0 Safari/600.00"
  wrk.headers["Cookie"] = "mrsession=43709dd361cc443e976b05714581a7fb; foo=fdsfdasdfasdfdsfasdfsdfsdfasdfas; short=fazc;"
  wrk.headers["Accept"] = "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"
  wrk.headers["Accept-Language"] = "en-US,en;q=0.5"
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))
  table.insert(r, wrk.format(nil, "/"))

  req = table.concat(r)
end

request = function()
   return req
end
