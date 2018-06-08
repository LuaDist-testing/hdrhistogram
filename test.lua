local hdrhistogram = require "hdrhistogram"
require "string"

local multiplier = 0.001

local hdr = hdrhistogram.new(1,1000000,3, {multiplier=multiplier, unit="ms"})

local expected_min    = 0
local expected_max    = 0
local expected_p50    = 0

for i,v in pairs(getmetatable(hdr)) do
  print(i,v)
end

assert(hdr:min() == expected_min, string.format("incorrect min value expected: %d, received %d", expected_min, hdr:min()))
assert(hdr:max() == expected_max, string.format("incorrect max value expected: %d, received %d", expected_max, hdr:max()))
assert(hdr:percentile(50) == expected_p50, string.format("incorrect p50 value expected: %d, received %d", expected_p50, hdr:percentile(50)))

for i = 10, 1000000, 10 do
  hdr:record(i*multiplier)
end

local expected_min    = 10*multiplier
local expected_max    = 1000447*multiplier
local expected_p50    = 500223*multiplier
print(hdr:min())
assert(hdr:min() == expected_min, string.format("incorrect min value expected: %f, received %f", expected_min, hdr:min()))
assert(hdr:max() == expected_max, string.format("incorrect max value expected: %f, received %f", expected_max, hdr:max()))
assert(hdr:percentile(50) == expected_p50, string.format("incorrect p50 value expected: %f, received %f", expected_p50, hdr:percentile(50)))

print(hdr:stats())
print(hdr:latency_stats())

print("OK")
