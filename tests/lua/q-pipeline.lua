-- example script demonstrating HTTP pipelining

init = function(args)
   local r = {}
   wrk.headers["Content-Type"] = "application/json; charset=utf-8"
   r[1]  = wrk.format('POST','/q/1/2/', {"Content-Type", "application/json"}, '{"my":"json"}')
   r[2]  = wrk.format('POST','/q/1/2/', {"Content-Type", "application/json"}, '{"my":"json"}')
   r[3]  = wrk.format('POST','/q/1/2/', {"Content-Type", "application/json"}, '{"my":"json"}')
   r[4]  = wrk.format('POST','/q/1/2/', {"Content-Type", "application/json"}, '{"my":"json"}')
   r[5]  = wrk.format('POST','/q/1/2/', {"Content-Type", "application/json"}, '{"my":"json"}')
   r[6]  = wrk.format('POST','/q/1/2/', {"Content-Type", "application/json"}, '{"my":"json"}')
   r[7]  = wrk.format('POST','/q/1/2/', {"Content-Type", "application/json"}, '{"my":"json"}')
   r[8]  = wrk.format('POST','/q/1/2/', {"Content-Type", "application/json"}, '{"my":"json"}')
   r[9]  = wrk.format('POST','/q/1/2/', {"Content-Type", "application/json"}, '{"my":"json"}')
   r[10] = wrk.format('POST','/q/1/2/', {"Content-Type", "application/json"}, '{"my":"json"}')
   r[11] = wrk.format('POST','/q/1/2/', {"Content-Type", "application/json"}, '{"my":"json"}')
   r[12] = wrk.format('POST','/q/1/2/', {"Content-Type", "application/json"}, '{"my":"json"}')
   r[13] = wrk.format('POST','/q/1/2/', {"Content-Type", "application/json"}, '{"my":"json"}')
   r[14] = wrk.format('POST','/q/1/2/', {"Content-Type", "application/json"}, '{"my":"json"}')
   r[15] = wrk.format('POST','/q/1/2/', {"Content-Type", "application/json"}, '{"my":"json"}')
   req = table.concat(r)
end

request = function()
   return req
end
