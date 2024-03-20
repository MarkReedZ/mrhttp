-- example script demonstrating HTTP pipelining

init = function(args)
   local r = {}
   wrk.headers["Content-Type"] = "application/mrpacker"
   wrk.headers["Cookie"] = "mrsession=43709dd361cc443e976b05714581a7fb"
   table.insert(r, wrk.format('POST','/mrq/1', nil, string.char(0x44,0xc1,0xc2,0xc3,0xc4)))
   table.insert(r, wrk.format('POST','/mrq/1', nil, string.char(0x44,0xc1,0xc2,0xc3,0xc4)))
   table.insert(r, wrk.format('POST','/mrq/1', nil, string.char(0x44,0xc1,0xc2,0xc3,0xc4)))
   table.insert(r, wrk.format('POST','/mrq/1', nil, string.char(0x44,0xc1,0xc2,0xc3,0xc4)))
   table.insert(r, wrk.format('POST','/mrq/1', nil, string.char(0x44,0xc1,0xc2,0xc3,0xc4)))
   table.insert(r, wrk.format('POST','/mrq/1', nil, string.char(0x44,0xc1,0xc2,0xc3,0xc4)))
   table.insert(r, wrk.format('POST','/mrq/1', nil, string.char(0x44,0xc1,0xc2,0xc3,0xc4)))
   table.insert(r, wrk.format('POST','/mrq/1', nil, string.char(0x44,0xc1,0xc2,0xc3,0xc4)))
   table.insert(r, wrk.format('POST','/mrq/1', nil, string.char(0x44,0xc1,0xc2,0xc3,0xc4)))
   table.insert(r, wrk.format('POST','/mrq/1', nil, string.char(0x44,0xc1,0xc2,0xc3,0xc4)))
   table.insert(r, wrk.format('POST','/mrq/1', nil, string.char(0x44,0xc1,0xc2,0xc3,0xc4)))
   table.insert(r, wrk.format('POST','/mrq/1', nil, string.char(0x44,0xc1,0xc2,0xc3,0xc4)))
   table.insert(r, wrk.format('POST','/mrq/1', nil, string.char(0x44,0xc1,0xc2,0xc3,0xc4)))
   table.insert(r, wrk.format('POST','/mrq/1', nil, string.char(0x44,0xc1,0xc2,0xc3,0xc4)))
   table.insert(r, wrk.format('POST','/mrq/1', nil, string.char(0x44,0xc1,0xc2,0xc3,0xc4)))
   req = table.concat(r)
end

request = function()
   return req
end

