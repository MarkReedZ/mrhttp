-- example script demonstrating HTTP pipelining

init = function(args)
   local r = {}
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   table.insert( r, wrk.format(nil, "/cached") )
   req = table.concat(r)
end

request = function()
   return req
end