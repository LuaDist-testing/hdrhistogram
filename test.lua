local hdrhistogram = require "hdrhistogram"
require "string"

local multiplier = 0.001

local hdr = hdrhistogram.new(1,1000000,3, {multiplier=multiplier, unit="ms"})

local expected_min    = 0
local expected_max    = 0
local expected_p50    = 0


assert(hdr:min() == expected_min, string.format("incorrect min value expected: %d, received %d", expected_min, hdr:min()))
assert(hdr:max() == expected_max, string.format("incorrect max value expected: %d, received %d", expected_max, hdr:max()))
assert(hdr:percentile(50) == expected_p50, string.format("incorrect p50 value expected: %d, received %d", expected_p50, hdr:percentile(50)))

for i = 10, 1000000, 10 do
  hdr:record(i*multiplier)
end

local expected_min    = 10*multiplier
local expected_max    = 1000447*multiplier
local expected_p50    = 500223*multiplier
assert(hdr:min() == expected_min, string.format("incorrect min value expected: %f, received %f", expected_min, hdr:min()))
assert(hdr:max() == expected_max, string.format("incorrect max value expected: %f, received %f", expected_max, hdr:max()))
assert(hdr:percentile(50) == expected_p50, string.format("incorrect p50 value expected: %f, received %f", expected_p50, hdr:percentile(50)))

--local json = require "cjson"

local ser = hdr:serialize()

local hdrcopy = hdrhistogram.unserialize(ser)

assert(hdr:stats() == hdrcopy:stats())
assert(tostring(hdr) == tostring(hdrcopy))


local hdr2 = hdrhistogram.new(1,1000000,3, {multiplier=multiplier/10, unit="ms"})

local ok, ret = pcall(hdr.merge, hdr, hdr2)
assert(not ok)

local hdr3 = hdrhistogram.new(1,1000000,3, {multiplier=multiplier, unit="mm"})
local ok, ret = pcall(hdr.merge, hdr, hdr3)
assert(not ok)

local hdr4 = hdrhistogram.new(1,1000000,3, {multiplier=multiplier, unit="ms"})
local ok, ret = pcall(hdr.merge, hdr, hdr4)
assert(ok)

print("OK")
