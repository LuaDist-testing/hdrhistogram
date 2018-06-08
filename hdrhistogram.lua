local hdr = require "hdrhistogram.hdr"

local hdrmeta = getmetatable(hdr.new(1, 1000, 2))

local new = hdr.new

local data = setmetatable({}, {
  __mode="k",
  __index=function(t, k)
    local ret = {multiplier=1, unit=""}
    t[k] = ret
    return ret
  end
})

function hdr.new(lowest, highest, sig, opt)
  
  opt = opt or {}
  local self = new(lowest, highest, sig)
  print(type(self))
  
  data[self] = {
    multiplier = opt.multiplier or 1,
    unit = opt.unit or opt.units or ""
  }
  
  return self
end

local record = hdrmeta.record
function hdrmeta:record(val)
  return record(self, val * 1/data[self].multiplier)
end

for i, v in ipairs({"min", "max", "mean", "stddev", "percentile"}) do
  local orig = hdrmeta[v]
  hdrmeta[v] = function(self, ...)
    return orig(self, ...) * data[self].multiplier
  end
end

local serialize = hdrmeta.serialize
function hdrmeta:serialize()
  local ret = serialize(self)
  ret["_multiplier"]=data[self].multiplier
  ret["_unit"]=data[self].unit
  return ret
end

local unserialize = hdr.unserialize
function hdr.unserialize(tbl)
  local new_hdr = unserialize(tbl)
  data[new_hdr]={
    multiplier= tbl._multiplier or 1,
    unit= tbl._unit or ""
  }
  return new_hdr
end

function hdrmeta:stats(percentiles)
  percentiles = percentiles or {10,20,30,40,50,60,70,80,90,100}
  local out = {}
  local pctf = (data[self].multiplier or 1) < 1 and  "%12." .. math.ceil(math.abs(math.log10(data[self].multiplier, 10))) .. "f" or "%12u"
  local fstr = "%7.3f%% "..pctf..data[self].unit
  for i,v in ipairs(percentiles) do
    table.insert(out, fstr:format(v, self:percentile(v)))
  end
  return table.concat(out, "\n")
end

function hdrmeta:latency_stats()
  local out = {
    "# Latency stats",
    self:stats { 50, 75, 90, 95, 99, 99.9, 100 }
  }
  return table.concat(out, "\n")
end

return hdr
