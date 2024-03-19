-- example script demonstrating HTTP pipelining

init = function(args)
   local r = {}
   wrk.headers["Content-Type"] = "application/json; charset=utf-8"
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   table.insert(r, wrk.format('POST','/json', {"Content-Type", "application/json"}, '{"name":"json"}'))
   req = table.concat(r)
end

request = function()
   return req
end
