-- example script demonstrating HTTP pipelining

init = function(args)
   local r = {}
   wrk.headers["Cookie"] = "mrsession=43709dd361cc443e976b05714581a7fb"
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   table.insert(r, wrk.format(nil, "/s"))
   req = table.concat(r)
end

request = function()
   return req
end
