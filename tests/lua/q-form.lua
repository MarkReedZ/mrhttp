-- example script demonstrating HTTP pipelining

init = function(args)
   local r = {}
   wrk.headers["Content-Type"] = "application/x-www-form-urlencoded"
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   table.insert(r, wrk.format('POST','/form', nil, 'param1=value1&param2=value2&c%C3%B3mo=puedes&fffffffffffffffffffffffffffffffffffff%20ffffffffffffffffff=aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa'))
   req = table.concat(r)
end

request = function()
   return req
end
