
function read_txt_file(path)
    local file, errorMessage = io.open(path, "r")
    if not file then 
        error("Could not read the file:" .. errorMessage .. "\n")
    end

    local content = file:read "*all"
    file:close()
    return content
end

local Boundary = "----WebKitFormBoundaryePkpFF7tjBAqx29L"
local BodyBoundary = "--" .. Boundary
local LastBoundary = "--" .. Boundary .. "--"

local CRLF = "\r\n"

local FileBody = read_txt_file("tests/lua/test.txt")

-- We don't need different file names here because the test should
-- always replace the uploaded file with the new one. This will avoid
-- the problem with directories having too much files and slowing down
-- the application, which is not what we are trying to test here.
-- This will also avoid overloading wrk with more things do to, which
-- can influence the test results.
local Filename = "test.txt"

local ContentDisposition = "Content-Disposition: form-data; name=\"file\"; filename=\"" .. Filename .. "\""

wrk.method = "POST"
wrk.headers["Content-Type"] = "multipart/form-data; boundary=" .. Boundary
wrk.body = BodyBoundary .. CRLF .. ContentDisposition .. CRLF .. CRLF .. FileBody .. CRLF .. LastBoundary .. CRLF

