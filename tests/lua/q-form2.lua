-- example script demonstrating HTTP pipelining

init = function(args)
   local r = {}
   wrk.headers["Content-Type"] = "application/x-www-form-urlencoded"
   r[1]  = wrk.format('POST','/form', {"Content-Type", "application/x-www-form-urlencoded"}, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')
   r[2]  = wrk.format('POST','/form', {"Content-Type", "application/x-www-form-urlencoded"}, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')
   r[3]  = wrk.format('POST','/form', {"Content-Type", "application/x-www-form-urlencoded"}, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')
   r[4]  = wrk.format('POST','/form', {"Content-Type", "application/x-www-form-urlencoded"}, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')
   r[5]  = wrk.format('POST','/form', {"Content-Type", "application/x-www-form-urlencoded"}, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')
   r[6]  = wrk.format('POST','/form', {"Content-Type", "application/x-www-form-urlencoded"}, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')
   r[7]  = wrk.format('POST','/form', {"Content-Type", "application/x-www-form-urlencoded"}, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')
   r[8]  = wrk.format('POST','/form', {"Content-Type", "application/x-www-form-urlencoded"}, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')
   r[9]  = wrk.format('POST','/form', {"Content-Type", "application/x-www-form-urlencoded"}, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')
   r[10]  = wrk.format('POST','/form', {"Content-Type", "application/x-www-form-urlencoded"}, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')
   r[11]  = wrk.format('POST','/form', {"Content-Type", "application/x-www-form-urlencoded"}, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')
   r[12]  = wrk.format('POST','/form', {"Content-Type", "application/x-www-form-urlencoded"}, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')
   r[13]  = wrk.format('POST','/form', {"Content-Type", "application/x-www-form-urlencoded"}, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')
   r[14]  = wrk.format('POST','/form', {"Content-Type", "application/x-www-form-urlencoded"}, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')
   r[15]  = wrk.format('POST','/form', {"Content-Type", "application/x-www-form-urlencoded"}, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa')
   req = table.concat(r)
end

request = function()
   return req
end
