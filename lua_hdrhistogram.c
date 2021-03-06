/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua HdrHistogram implementation @file */

#include <stdlib.h>
#include <string.h>

#include <stdio.h>
#include <assert.h>

#include <lua.h>
#include <lauxlib.h>

#include "hdr_histogram.h"

#ifdef LUA_SANDBOX
#include "luasandbox_output.h"
#include "luasandbox_serialize.h"
#endif

static const char* LHDR = "hdr.histogram";


static int lhdr_new(lua_State* lua)
{
  int n = lua_gettop(lua);
  luaL_argcheck(lua, n == 3, 0, "incorrect number of arguments");

  int64_t lowest_trackable_value = luaL_checknumber(lua, 1);
  int64_t highest_trackable_value = luaL_checknumber(lua, 2);
  lua_Integer significant_figures = luaL_checkinteger(lua, 3);
  luaL_argcheck(lua, lowest_trackable_value >= 1,
                1, "lowest trackable value must be >= 1");
  luaL_argcheck(lua, highest_trackable_value > lowest_trackable_value * 2,
                2, "highest trackable value must be > lowest*2");
  luaL_argcheck(lua, 0 < significant_figures && 6 > significant_figures,
                3, "significant figures must be 1-5");

  struct hdr_histogram_bucket_config cfg;
  int r = hdr_calculate_bucket_config(lowest_trackable_value,
                                      highest_trackable_value,
                                      significant_figures,
                                      &cfg);
  if (r) luaL_error(lua, "hdr_calculate_bucket_config failed");

  size_t histogram_size = sizeof(struct hdr_histogram) + cfg.counts_len
    * sizeof(int64_t);
  struct hdr_histogram* hdr = lua_newuserdata(lua, histogram_size);
  memset(hdr, 0, histogram_size);
  hdr_init_preallocated(hdr, &cfg);
  luaL_getmetatable(lua, LHDR);
  lua_setmetatable(lua, -2);

  return 1;
}


static struct hdr_histogram* check_hdrhistogram(lua_State* lua, int args)
{
  struct hdr_histogram* hdr = luaL_checkudata(lua, 1, LHDR);
  luaL_argcheck(lua, args == lua_gettop(lua), 0,
                "incorrect number of arguments");
  return hdr;
}


static int lhdr_memsize(lua_State* lua)
{
  struct hdr_histogram* hdr = check_hdrhistogram(lua, 1);
  size_t mem_size = 0;
  mem_size = hdr_get_memory_size(hdr);
  lua_pushnumber(lua, mem_size);
  return 1;
}


static int lhdr_count(lua_State* lua)
{
  struct hdr_histogram* hdr = check_hdrhistogram(lua, 1);
  lua_pushnumber(lua, hdr->total_count);
  return 1;
}


static int lhdr_record_value(lua_State* lua)
{
  struct hdr_histogram* hdr = check_hdrhistogram(lua, 2);
  int64_t value = luaL_checknumber(lua, 2);
  bool success = hdr_record_value(hdr, value);
  lua_pushboolean(lua, success);
  return 1;
}

static int lhdr_record_corrected_value(lua_State* lua)
{
  struct hdr_histogram* hdr = check_hdrhistogram(lua, 3);
  int64_t value = luaL_checknumber(lua, 2);
  int64_t interval = luaL_checknumber(lua, 3);
  bool success = hdr_record_corrected_value(hdr, value, interval);
  lua_pushboolean(lua, success);
  return 1;
}

static int lhdr_min(lua_State* lua)
{
  struct hdr_histogram* hdr = check_hdrhistogram(lua, 1);
  int64_t min = 0;
  if (hdr->total_count > 0) {
    min = hdr_min(hdr);
  }
  lua_pushnumber(lua, min);
  return 1;
}


static int lhdr_max(lua_State* lua)
{
  struct hdr_histogram* hdr = check_hdrhistogram(lua, 1);
  int64_t max = 0;
  if (hdr->total_count > 0) {
    max = hdr_max(hdr);
  }
  lua_pushnumber(lua, max);
  return 1;
}


static int lhdr_mean(lua_State* lua)
{
  struct hdr_histogram* hdr = check_hdrhistogram(lua, 1);
  double mean = 0.0;
  if (hdr->total_count > 0) {
    mean = hdr_mean(hdr);
  }
  lua_pushnumber(lua, mean);
  return 1;
}


static int lhdr_stddev(lua_State* lua)
{
  struct hdr_histogram* hdr = check_hdrhistogram(lua, 1);
  double stddev = 0.0;
  if (hdr->total_count > 0) {
    stddev = hdr_stddev(hdr);
  }
  lua_pushnumber(lua, stddev);
  return 1;
}


static int lhdr_percentile(lua_State* lua)
{
  struct hdr_histogram* hdr = check_hdrhistogram(lua, 2);
  double percentile = luaL_checknumber(lua, 2);
  int64_t value = 0;
  value = hdr_value_at_percentile(hdr, percentile);
  lua_pushnumber(lua, value);
  return 1;
}


static int lhdr_merge(lua_State* lua)
{
  struct hdr_histogram* hdr = check_hdrhistogram(lua, 2);
  struct hdr_histogram* hdr1 = luaL_checkudata(lua, 2, LHDR);
  int64_t dropped = 0;
  dropped = hdr_add(hdr, hdr1);
  lua_pushnumber(lua, dropped);
  return 1;
}


static int lhdr_reset(lua_State* lua)
{
  struct hdr_histogram* hdr = check_hdrhistogram(lua, 1);
  hdr_reset(hdr);
  return 0;
}


static int lhdr_version(lua_State* lua)
{
  lua_pushstring(lua, DIST_VERSION);
  return 1;
}


static int lhdr_tostring(lua_State* lua)
{
  struct hdr_histogram* hdr = check_hdrhistogram(lua, 1);

  lua_pushfstring(lua, "lowest_trackable_value:  %d\n"
                  "highest_trackable_value: %d\n"
                  "significant_figures:     %d\n"
                  "unit_magnitude:          %d\n"
                  "total_count:             %d\n"
                  "bucket_count:            %d\n"
                  "sub_bucket_count:        %d\n"
                  "counts_len:              %d",
                  hdr->lowest_trackable_value,
                  hdr->highest_trackable_value,
                  hdr->significant_figures,
                  hdr->unit_magnitude,
                  hdr->total_count,
                  hdr->bucket_count,
                  hdr->sub_bucket_count,
                  hdr->counts_len);
//  hdr_percentiles_print(hdr, stdout, 5, 1.0, CLASSIC);
  return 1;
}

#define add_hdr_property_to_table(lua, hdr, propname) \
  lua_pushnumber(lua, hdr->propname);                 \
  lua_setfield(lua, -2, #propname);

static int lhdr_serialize(lua_State *lua) {
  struct hdr_histogram* hdr = check_hdrhistogram(lua, 1);
  
  lua_newtable(lua);
  
  add_hdr_property_to_table(lua, hdr, lowest_trackable_value)
  add_hdr_property_to_table(lua, hdr, highest_trackable_value)
  add_hdr_property_to_table(lua, hdr, unit_magnitude);
  add_hdr_property_to_table(lua, hdr, significant_figures);
  add_hdr_property_to_table(lua, hdr, sub_bucket_half_count_magnitude);
  add_hdr_property_to_table(lua, hdr, sub_bucket_half_count);
  add_hdr_property_to_table(lua, hdr, sub_bucket_mask);
  add_hdr_property_to_table(lua, hdr, sub_bucket_count);
  add_hdr_property_to_table(lua, hdr, bucket_count);
  add_hdr_property_to_table(lua, hdr, min_value);
  add_hdr_property_to_table(lua, hdr, max_value);
  add_hdr_property_to_table(lua, hdr, normalizing_index_offset);
  add_hdr_property_to_table(lua, hdr, conversion_ratio);
  add_hdr_property_to_table(lua, hdr, counts_len);
  add_hdr_property_to_table(lua, hdr, total_count);
  
  
  lua_newtable(lua);
  
  int i;
  for(i = 0; i<hdr->counts_len; i++) {
    lua_pushinteger(lua, hdr->counts[i]);
    lua_rawseti(lua, -2, i+1);
  }
  
  lua_setfield(lua, -2, "counts");
  
  return 1;
}

#define unserialize_hdr_int_property(lua, hdr, propname, type) \
  lua_getfield(lua, 1, #propname)           ;                \
  hdr->propname = (type )lua_tonumber(lua, -1);               \
  lua_pop(lua, 1);                                            

#define unserialize_hdr_float_property(lua, hdr, propname, type) \
  lua_getfield(lua, 1, #propname)           ;                   \
  hdr->propname = (type )lua_tonumber(lua, -1);                  \
  lua_pop(lua, 1);                                           

static int lhdr_unserialize(lua_State *lua) {
  struct hdr_histogram*  hdr;
  lua_Integer            counts_len;
  size_t                 histogram_size;
  int                    i;
  
  assert(lua_istable(lua, 1));
  lua_getfield(lua, 1, "counts_len");
  counts_len = lua_tointeger(lua, -1);
  
  assert(lua_istable(lua, 1));
  
  histogram_size = sizeof(struct hdr_histogram) + counts_len * sizeof(int64_t);
  hdr = lua_newuserdata(lua, histogram_size);
  
  unserialize_hdr_int_property(lua, hdr, lowest_trackable_value, int64_t);
  unserialize_hdr_int_property(lua, hdr, highest_trackable_value, int64_t);
  unserialize_hdr_int_property(lua, hdr, unit_magnitude, int32_t);
  unserialize_hdr_int_property(lua, hdr, significant_figures, int64_t);
  unserialize_hdr_int_property(lua, hdr, sub_bucket_half_count_magnitude, int32_t);
  unserialize_hdr_int_property(lua, hdr, sub_bucket_half_count, int32_t);
  unserialize_hdr_int_property(lua, hdr, sub_bucket_mask, int64_t);
  unserialize_hdr_int_property(lua, hdr, sub_bucket_count, int32_t);
  unserialize_hdr_int_property(lua, hdr, bucket_count, int32_t);
  unserialize_hdr_int_property(lua, hdr, min_value, int64_t);
  unserialize_hdr_int_property(lua, hdr, max_value, int64_t);
  unserialize_hdr_int_property(lua, hdr, normalizing_index_offset, int32_t);
  unserialize_hdr_float_property(lua, hdr, conversion_ratio, double);
  hdr->counts_len = (int32_t )counts_len;
  unserialize_hdr_int_property(lua, hdr, total_count, int64_t);
  
  lua_getfield(lua, 1, "counts");
  for(i=0; i<counts_len; i++) {
    lua_rawgeti(lua, -1, i+1);
    hdr->counts[i]=(int64_t )lua_tointeger(lua, -1);
    lua_pop(lua, 1);
  }
  lua_pop(lua, 1);
  
  luaL_getmetatable(lua, LHDR);
  lua_setmetatable(lua, -2);
  
  return 1;
}


static const struct luaL_Reg lhdr_functions[] = {
  { "new", lhdr_new },
  { "version", lhdr_version },
  { "unserialize", lhdr_unserialize },
  { NULL, NULL }
};


static const struct luaL_Reg lhdr_methods[] = {
  { "reset", lhdr_reset },
  { "memsize", lhdr_memsize },
  { "count", lhdr_count },
  { "record", lhdr_record_value },
  { "record_corrected", lhdr_record_corrected_value },
  { "min", lhdr_min },
  { "max", lhdr_max },
  { "mean", lhdr_mean },
  { "stddev", lhdr_stddev },
  { "percentile", lhdr_percentile },
  { "merge", lhdr_merge },
  { "serialize", lhdr_serialize },
  { "__tostring", lhdr_tostring },
  { NULL, NULL },
};


int luaopen_hdrhistogram_hdr(lua_State* lua)
{
  luaL_newmetatable(lua, LHDR);
  lua_pushvalue(lua, -1);
  lua_setfield(lua, -2, "__index");

#if LUA_VERSION_NUM > 501
  luaL_setfuncs(lua, lhdr_methods, 0);
  
  lua_newtable(lua);
  luaL_setfuncs(lua,lhdr_functions,0);
  //lua_pushvalue(lua,-1);        // pluck these lines out if they offend you
  //lua_setglobal(lua,"hdrhistogram"); // for they clobber the Holy _G
#else
  luaL_register(lua, NULL, lhdr_methods);
  lua_newtable(lua);
  luaL_register(lua, NULL, lhdr_functions);
#endif  
  return 1;
}
