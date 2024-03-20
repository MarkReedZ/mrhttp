-- File upload pipelined

function read_txt_file(path)
    local file, errorMessage = io.open(path, "r")
    if not file then 
        error("Could not read the file:" .. errorMessage .. "\n")
    end

    local content = file:read "*all"
    file:close()
    return content
end

init = function(args)
  local r = {}
  local Boundary = "----WebKitFormBoundaryePkpFF7tjBAqx29L"
  local BodyBoundary = "--" .. Boundary
  local LastBoundary = "--" .. Boundary .. "--"
  
  local CRLF = "\r\n"
  
  local FileBody = read_txt_file("tests/lua/test.txt")
  
  local Filename = "test.txt"
  
  local ContentDisposition = "Content-Disposition: form-data; name=\"file\"; filename=\"" .. Filename .. "\""
  
  wrk.method = "POST"
  wrk.headers["Content-Type"] = "multipart/form-data; boundary=" .. Boundary
  --wrk.headers["Cookie"] = "mrsession=43709dd361cc443e976b05714581a7fb"
  local body = BodyBoundary .. CRLF .. ContentDisposition .. CRLF .. CRLF .. FileBody .. CRLF .. LastBoundary .. CRLF

  table.insert(r, wrk.format(nil, "/", nil, body))
  table.insert(r, wrk.format(nil, "/", nil, body))
  table.insert(r, wrk.format(nil, "/", nil, body))
  table.insert(r, wrk.format(nil, "/", nil, body))
  table.insert(r, wrk.format(nil, "/", nil, body))
  table.insert(r, wrk.format(nil, "/", nil, body))
  table.insert(r, wrk.format(nil, "/", nil, body))
  table.insert(r, wrk.format(nil, "/", nil, body))
  table.insert(r, wrk.format(nil, "/", nil, body))
  req = table.concat(r)
end

request = function()
   return req
end


